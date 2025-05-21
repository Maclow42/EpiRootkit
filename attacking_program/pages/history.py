# --------------------------------- HISTORY --------------------------------- #

@app.route('/history')
def history():
    if not authenticated:
        return redirect(url_for('login'))
    return render_template("history.html", history=command_history)