from app import app
import config as cfg
from flask import render_template, redirect, url_for


# -------------------------------- DASHBOARD -------------------------------- #

@app.route('/dashboard')
def dashboard():
    if not cfg.authenticated:
        return redirect(url_for('login'))
    status = "✅ Connecté" if cfg.rootkit_connection else "🔴 Déconnecté"
    return render_template("dashboard.html", status=status, rootkit_address=cfg.rootkit_address)
