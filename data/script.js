// script.js
let isScanning = false;
let networks = [];
let wifiNetworks = [];

// Khởi tạo khi trang được load
window.onload = function() {
    updateStatus('Ready to scan networks');
    // Tự động scan khi load trang
    setTimeout(scanNetworks, 1000);
};

function updateStatus(message) {
    const statusElement = document.getElementById('status');
    statusElement.textContent = message;
}

function fetchWifiNetworks() {
    fetch('/api/wifi-networks')
        .then(response => response.json())
        .then(data => {
            wifiNetworks = data.networks;
            displayWifiNetworks();
        })
        .catch(error => console.error('Error fetching Wi-Fi networks:', error));
}

function displayWifiNetworks() {
    const networkList = document.getElementById('network-list');
    networkList.innerHTML = '';

    wifiNetworks.forEach(network => {
        const listItem = document.createElement('li');
        listItem.textContent = `${network.ssid} (${network.signalStrength} dBm)`;
        
        const deauthButton = document.createElement('button');
        deauthButton.textContent = 'Deauth';
        deauthButton.onclick = () => deauthenticateNetwork(network.ssid);
        
        listItem.appendChild(deauthButton);
        networkList.appendChild(listItem);
    });
}

function scanNetworks() {
    if (isScanning) return;
    
    isScanning = true;
    const scanBtn = document.getElementById('scanBtn');
    scanBtn.disabled = true;
    scanBtn.innerHTML = '<span>⏳</span> Scanning...';
    
    updateStatus('Scanning for networks...');
    document.getElementById('networks').innerHTML = '<div class="loading">🔍 Scanning networks, please wait...</div>';
    
    fetch('/scan')
        .then(response => {
            if (!response.ok) throw new Error('Scan failed');
            updateStatus('Scan initiated, getting results...');
            // Đợi 3 giây để scan hoàn thành
            setTimeout(getNetworks, 3000);
        })
        .catch(error => {
            console.error('Scan error:', error);
            updateStatus('❌ Scan failed. Please try again.');
            resetScanButton();
        });
}

function getNetworks() {
    fetch('/networks')
        .then(response => {
            if (!response.ok) throw new Error('Failed to get networks');
            return response.json();
        })
        .then(data => {
            networks = data;
            displayNetworks(data);
            updateStatus(`Found ${data.length} network(s)`);
            resetScanButton();
        })
        .catch(error => {
            console.error('Get networks error:', error);
            updateStatus('❌ Failed to get network list');
            document.getElementById('networks').innerHTML = '<div class="no-networks">Failed to load networks</div>';
            resetScanButton();
        });
}

function resetScanButton() {
    isScanning = false;
    const scanBtn = document.getElementById('scanBtn');
    scanBtn.disabled = false;
    scanBtn.innerHTML = '<span>🔍</span> Scan Networks';
}

function displayNetworks(networks) {
    const networksDiv = document.getElementById('networks');
    
    if (networks.length === 0) {
        networksDiv.innerHTML = '<div class="no-networks">No networks found</div>';
        return;
    }
    
    let html = '';
    networks.forEach((network, index) => {
        const signalClass = getSignalClass(network.rssi);
        const encryptionBadge = getEncryptionBadge(network.encryption || 0);
        
        html += `
            <div class="network">
                <div class="network-info">
                    <div class="network-name">${escapeHtml(network.ssid || 'Hidden Network')}</div>
                    <div class="network-details">
                        Signal: ${network.rssi} dBm
                        <span class="signal-strength ${signalClass}">${getSignalText(network.rssi)}</span>
                        ${encryptionBadge}
                    </div>
                </div>
                <div class="network-actions">
                    <button class="deauth-btn" onclick="deauth('${escapeHtml(network.ssid)}', ${index})">
                        ⚡ Deauth
                    </button>
                </div>
            </div>
        `;
    });
    
    networksDiv.innerHTML = html;
}

function getSignalClass(rssi) {
    if (rssi > -50) return 'signal-excellent';
    if (rssi > -60) return 'signal-good';
    if (rssi > -70) return 'signal-fair';
    return 'signal-poor';
}

function getSignalText(rssi) {
    if (rssi > -50) return 'Excellent';
    if (rssi > -60) return 'Good';
    if (rssi > -70) return 'Fair';
    return 'Poor';
}

function getEncryptionBadge(encType) {
    switch(encType) {
        case 0: return '<span class="encryption-badge encryption-open">OPEN</span>';
        case 1: return '<span class="encryption-badge encryption-wep">WEP</span>';
        case 2: return '<span class="encryption-badge encryption-wpa">WPA</span>';
        case 3: return '<span class="encryption-badge encryption-wpa">WPA2</span>';
        case 4: return '<span class="encryption-badge encryption-wpa">WPA/WPA2</span>';
        case 5: return '<span class="encryption-badge encryption-wpa">WPA2 Enterprise</span>';
        default: return '<span class="encryption-badge">Unknown</span>';
    }
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

function deauth(ssid, index) {
    if (!ssid) {
        alert('Cannot deauth hidden network');
        return;
    }
    
    const confirmed = confirm(`Are you sure you want to deauth network: ${ssid}?\n\nThis will disconnect all devices from this network.`);
    
    if (confirmed) {
        updateStatus(`🚫 Deauthing ${ssid}...`);
        
        // Gửi request deauth đến server
        fetch('/deauth', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                ssid: ssid,
                index: index
            })
        })
        .then(response => response.text())
        .then(data => {
            updateStatus(`✅ Deauth attack sent to ${ssid}`);
            console.log('Deauth response:', data);
        })
        .catch(error => {
            console.error('Deauth error:', error);
            updateStatus(`❌ Failed to deauth ${ssid}`);
        });
    }
}

// Tự động refresh networks mỗi 30 giây
setInterval(() => {
    if (!isScanning) {
        getNetworks();
    }
}, 30000);

// Call fetchWifiNetworks on page load
window.onload = fetchWifiNetworks;