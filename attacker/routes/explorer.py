from app import app
from flask import render_template, redirect, url_for, session
from datetime import datetime
from routes.api import getClientInfos
import config as cfg
import os

@app.route('/explorer')
def explorer():
    files = os.listdir(cfg.DOWNLOAD_FOLDER)
    return render_template("explorer.html", files=files, download_folder=cfg.DOWNLOAD_FOLDER)