from app import app
import config as cfg
from flask import render_template, redirect, url_for, request, flash
import os
import binascii
# --------------------------------- LISTE DES FICHIERS --------------------------------- #


@app.route('/download')
def download():
    files = os.listdir(cfg.DOWNLOAD_FOLDER)
    return render_template("download.html", files=files, download_folder=cfg.DOWNLOAD_FOLDER)

