from flask import Blueprint, render_template, redirect, url_for
from utils.tools import authenticated, rootkit_connection, rootkit_address

dashboard_bp = Blueprint('dashboard', __name__)


@dashboard_bp.route('/dashboard')
def dashboard():
    if not authenticated:
        return redirect(url_for('auth.login'))

    status = "✅ Connecté" if rootkit_connection else "🔴 Déconnecté"
    return render_template("dashboard.html", status=status, rootkit_address=rootkit_address)
