from flask import Blueprint, render_template, redirect, url_for, send_file
import utils.state as state
import os

download_bp = Blueprint('download', __name__)

@download_bp.route('/')
def download():
    if not state.authenticated:
        return redirect(url_for('auth.login'))
    files = os.listdir(state.DOWNLOAD_FOLDER)
    return render_template("download.html", files=files)

@download_bp.route('/<filename>')
def download_file(filename):
    if not state.authenticated:
        return redirect(url_for('auth.login'))
    return send_file(os.path.join(state.DOWNLOAD_FOLDER, filename), as_attachment=True)
