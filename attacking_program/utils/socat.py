import subprocess
import shutil

def run_socat_shell(port=9001):
    terminal_cmd = None

    if shutil.which("gnome-terminal"):
        terminal_cmd = [
            "gnome-terminal", "--", "bash", "-c",
            f"socat openssl-listen:{port},reuseaddr,cert=$(pwd)/server.pem,verify=0 file:`tty`,raw,echo=0; exec bash"
        ]
    elif shutil.which("xterm"):
        terminal_cmd = [
            "xterm", "-e",
            f"bash -c 'socat openssl-listen:{port},reuseaddr,cert=$(pwd)/server.pem,verify=0 file:`tty`,raw,echo=0; exec bash'"
        ]
    elif shutil.which("konsole"):
        terminal_cmd = [
            "konsole", "-e", "bash", "-c",
            f"socat openssl-listen:{port},reuseaddr,cert=$(pwd)/server.pem,verify=0 file:`tty`,raw,echo=0; exec bash"
        ]
    else:
        print("❌ Aucun terminal compatible trouvé.")
        return

    try:
        subprocess.Popen(terminal_cmd)
        print(f"🚀 Terminal distant lancé sur le port {port}.")
    except Exception as e:
        print(f"💥 Erreur socat : {e}")
