from app import app
from flask import render_template, redirect, url_for, session
from datetime import datetime

# -------------------------------- DASHBOARD -------------------------------- #

@app.route('/dashboard')
def dashboard():
    return render_template("dashboard.html")
