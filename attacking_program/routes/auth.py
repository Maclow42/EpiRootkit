from flask import Blueprint, request, render_template, redirect, url_for, flash
from attacking_program.config import PASSWORD
from attacking_program.utils.tools import authenticated

auth_bp = Blueprint('auth', __name__)


@auth_bp.route('/')
def login():
    if authenticated:
        return redirect(url_for('dashboard.dashboard'))
    return render_template("login.html")


@auth_bp.route('/auth', methods=['POST'])
def auth():
    global authenticated
    if request.form.get('password') == PASSWORD:
        authenticated = True
        return redirect(url_for('dashboard.dashboard'))
    flash("❌ Mot de passe incorrect.")
    return redirect(url_for('auth.login'))
