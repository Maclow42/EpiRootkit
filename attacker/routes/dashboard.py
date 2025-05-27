from app import app
from flask import render_template, redirect, url_for, session
from datetime import datetime
from routes.api import getClientInfos
import config as cfg

# -------------------------------- DASHBOARD -------------------------------- #

@app.route('/dashboard')
def dashboard():
    clientInfos = getClientInfos()
    return render_template("dashboard.html", clientInfos=clientInfos)
