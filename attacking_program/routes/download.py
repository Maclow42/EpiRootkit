from flask import Blueprint, render_template, redirect, url_for, send_file
from utils.state import authenticated, DOWNLOAD_FOLDER
import os

download_bp = Blueprint('download', __name__)

@download_bp.route('/')
def download():
    if not authenticated:
        return redirect(url_for('auth.login'))
    files = os.listdir(DOWNLOAD_FOLDER)
    return render_template("download.html", files=files)

@download_bp.route('/<filename>')
def download_file(filename):
    if not authenticated:
        return redirect(url_for('auth.login'))
    return send_file(os.path.join(DOWNLOAD_FOLDER, filename), as_attachment=True)
