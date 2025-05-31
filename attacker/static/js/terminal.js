const URL_EXEC = "/send";
const URL_HISTORY = "/get-history";

// Shortcuts to DOM elements
const elements = {
    commandInput: document.getElementById('commandInput'),
    channelInput: document.getElementById('channelInput'),
    lastResultBlock: document.getElementById('last-result-block'),
    lastStdout: document.getElementById('last-stdout'),
    lastStderr: document.getElementById('last-stderr'),
    lastReturnCode: document.getElementById('last-return-code'),
    historyList: document.querySelector('.history-list'),
    resultTitle: document.getElementById('last-result-title'),
    stdsBlock: document.getElementById('stds-code-block'),
    returnCodeBlock: document.querySelector('.return-code-block'),
    errorMessage: document.querySelector('.error-message'),
    sendButton: document.getElementById('send-cmd-button')
};

// Displays an error message in the UI
function showError(message) {
    if (!elements.errorMessage) return;
    elements.errorMessage.textContent = "❌ " + message;
    elements.errorMessage.classList.remove('hidden');
}

async function handleSend() {
    if (!elements.errorMessage.classList.contains('hidden')) {
        elements.errorMessage.classList.add('hidden');
    }

    const command = elements.commandInput.value.trim();
    const channel = elements.channelInput.value;

    if (!command || !channel) {
        showError("Command or channel is missing.");
        return;
    }

    const originalText = elements.sendButton.textContent;
    elements.sendButton.disabled = true;
    elements.sendButton.classList.add('loading');
    elements.sendButton.textContent = '⏳ Exécution...';

    try {
        await fetch(URL_EXEC, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ command, channel, use_history: true })
        });

        await retrieveHistory();
    } catch (err) {
        console.error('Failed to send command:', err);
        showError("Failed to send the command.");
    } finally {
        elements.sendButton.disabled = false;
        elements.sendButton.classList.remove('loading');
        elements.sendButton.textContent = originalText;
    }
}


// Retrieves the history and renders it in the DOM
async function retrieveHistory() {
    try {
        const res = await fetch(URL_HISTORY);
        const data = await res.json();

        renderHistory(data);
        renderLastCommand(data[data.length - 1] || null);
        toggleLastCommandDisplay();
        elements.commandInput.value = '';


        if (data.tcp_error) {
            showError(data.tcp_error);
        }


    } catch (err) {
        console.error('Error fetching history:', err);
        showError("Error while loading the history.");
    }
}

// Generates the full history display
function renderHistory(data) {
    elements.historyList.innerHTML = '';

    for (let i = data.length - 1; i >= 0; i--) {
        const cmd = data[i];
        const li = document.createElement('li');
        li.innerHTML = `
            <details>
                <summary><code>${escapeHtml(cmd.command)}</code></summary>
                <div class="command-details">
                    <div class="stdsdiv">
                        <div class="stdout">
                            <h5>🟢 stdout:</h5>
                            <pre>${escapeHtml(cmd.stdout)}</pre>
                        </div>
                        <div class="stderr">
                            <h5>🔴 stderr:</h5>
                            <pre>${escapeHtml(cmd.stderr)}</pre>
                        </div>
                    </div>
                    ${cmd.termination_code ? `<pre>➡️ Terminated with code: ${cmd.termination_code}</pre>` : ''}
                </div>
            </details>`;
        elements.historyList.appendChild(li);
    }
}

// Displays the last executed command
function renderLastCommand(cmd) {
    elements.lastStdout.textContent = cmd?.stdout || '';
    elements.lastStderr.textContent = cmd?.stderr || '';
    elements.lastReturnCode.textContent = cmd?.termination_code
        ? `➡️ Terminated with code: ${cmd.termination_code}`
        : '';
    toggleLastCommandDisplay();

    elements.lastResultBlock.classList.remove('growFromLine');
    void elements.lastResultBlock.offsetWidth; // !! needed to reset the animation
    elements.lastResultBlock.classList.add('growFromLine');

}

// Shows or hides the block displaying the results of the last command
function toggleLastCommandDisplay() {
    const hasStdout = elements.lastStdout.textContent.trim() !== '';
    const hasStderr = elements.lastStderr.textContent.trim() !== '';

    const visible = hasStdout || hasStderr;

    elements.lastResultBlock.style.display = visible ? 'block' : 'none';

    elements.returnCodeBlock.style.display = visible && elements.lastReturnCode.textContent.length > 0 ? 'block' : 'none';
}

// HTML escaping function to prevent XSS injections
function escapeHtml(str) {
    if (typeof str !== 'string') return '';
    return str
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
}

// Initialization
toggleLastCommandDisplay();
retrieveHistory();