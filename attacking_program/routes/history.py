from flask import Blueprint, render_template, redirect, url_for
import utils.state as state

history_bp = Blueprint('history', __name__)

@history_bp.route('/')
def history():
    if not state.authenticated:
        return redirect(url_for('auth.login'))
    return render_template("history.html", history=state.command_history)
