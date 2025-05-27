from app import app
from flask import request, redirect, url_for, flash, session
import config as cfg

# --------------------------------- WEB AUTH --------------------------------- #

@app.route('/auth', methods=['POST'])
def auth():
    if request.form.get('password') == cfg.PASSWORD:
        # ✅ Enregistre l'utilisateur comme authentifié dans la session
        session['authenticated'] = True
        return redirect(url_for('dashboard'))

    flash("❌ Mot de passe incorrect.")
    return redirect(url_for('login'))
