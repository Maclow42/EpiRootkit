from flask import Blueprint, render_template, request, redirect, url_for, flash
from utils.state import PASSWORD, authenticated, app

auth_bp = Blueprint('auth', __name__)

@auth_bp.route('/')
def login():
    from utils.state import authenticated
    if authenticated:
        return redirect(url_for('dashboard.dashboard'))
    return render_template("login.html")

@auth_bp.route('/auth', methods=['POST'])
def auth():
    from utils.state import authenticated
    if request.form.get('password') == PASSWORD:
        authenticated = True
        return redirect(url_for('dashboard.dashboard'))
    flash("❌ Mot de passe incorrect.")
    return redirect(url_for('auth.login'))
