from app import app
import config as cfg
from flask import render_template, redirect, url_for

# --------------------------------- HISTORY --------------------------------- #

@app.route('/history')
def history():
    if not cfg.authenticated:
        return redirect(url_for('login'))
    return render_template("history.html", history=cfg.command_history)