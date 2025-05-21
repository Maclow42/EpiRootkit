from app import app
from flask import request, redirect, url_for, flash

# --------------------------------- WEB AUTH --------------------------------- #

@app.route('/auth', methods=['POST'])
def auth():
    global authenticated
    if request.form.get('password') == PASSWORD:
        authenticated = True
        return redirect(url_for('dashboard'))
    flash("❌ Mot de passe incorrect.")
    return redirect(url_for('login'))
