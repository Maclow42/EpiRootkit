import os
from flask import Blueprint, render_template, redirect, url_for, send_file
from utils.tools import authenticated
from config import DOWNLOAD_FOLDER

download_bp = Blueprint('download', __name__)


@download_bp.route('/download')
def download():
    if not authenticated:
        return redirect(url_for('auth.login'))

    files = os.listdir(DOWNLOAD_FOLDER)
    return render_template("download.html", files=files)


@download_bp.route('/download/<filename>')
def download_file(filename):
    if not authenticated:
        return redirect(url_for('auth.login'))

    filepath = os.path.join(DOWNLOAD_FOLDER, filename)
    return send_file(filepath, as_attachment=True)
