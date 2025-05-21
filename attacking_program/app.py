import os
from flask import Flask
import config as cfg

# ------------------------------ FLASK SETUP -------------------------------- #

app = Flask(__name__)
app.secret_key = "epirootkit_secret"
app.config['UPLOAD_FOLDER'] = cfg.UPLOAD_FOLDER

os.makedirs(cfg.UPLOAD_FOLDER, exist_ok=True)
os.makedirs(cfg.DOWNLOAD_FOLDER, exist_ok=True)