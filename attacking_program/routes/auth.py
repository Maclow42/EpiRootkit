from flask import Blueprint, render_template, request, redirect, url_for, flash
import utils.state as state

auth_bp = Blueprint('auth', __name__)

@auth_bp.route('/')
def login():
    if state.authenticated:
        return redirect(url_for('dashboard.dashboard'))
    return render_template("login.html")

@auth_bp.route('/auth', methods=['POST'])
def auth():
    if request.form.get('password') == state.PASSWORD:
        state.authenticated = True
        return redirect(url_for('dashboard.dashboard'))
    flash("❌ Mot de passe incorrect.")
    return redirect(url_for('auth.login'))
