from flask import Blueprint, render_template, redirect, url_for
from utils.tools import authenticated, command_history

history_bp = Blueprint('history', __name__)


@history_bp.route('/history')
def history():
    if not authenticated:
        return redirect(url_for('auth.login'))

    return render_template("history.html", history=command_history)
