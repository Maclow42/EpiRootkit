from app import app
import config as cfg
from flask import render_template, redirect, url_for, session

# -------------------------------- KEYLOGGER -------------------------------- #

@app.route('/keylogger')
def keylogger():
    klg_on = cfg.rootkit_connexion.is_klg_on()
    print(f"[KLG] Keylogger is {'ON' if klg_on else 'OFF'}")
    return render_template("keylogger.html", klg_on=klg_on)
