// Extract value from protobuf JSON format (e.g., {int32_value: 5} -> 5)
function extractValue(protoVal) {
    if (!protoVal) return null;
    if ('int32_value' in protoVal) return protoVal.int32_value;
    if ('uint32_value' in protoVal) return protoVal.uint32_value;
    if ('float32_value' in protoVal) return protoVal.float32_value;
    if ('string_value' in protoVal) return protoVal.string_value;
    return null;
}

// =====================================================================
// Device Section
// =====================================================================
// Cached so the "Expand" button can re-render the most recent fetch in a
// modal without an extra round-trip.
let lastDeviceData = null;
let lastDeviceSlot = null;

async function selectSlot(slot) {
    // Update active button
    document.querySelectorAll('.slot-btn').forEach(btn => {
        btn.classList.toggle('active', parseInt(btn.dataset.slot) === slot);
    });
    
    // Show loading state
    const details = document.getElementById('deviceDetails');
    details.innerHTML = '<div class="device-loading">Loading...</div>';
    
    // Fetch device info
    await fetchDevice(slot);
}

async function fetchDevice(slot) {
    const details = document.getElementById('deviceDetails');
    try {
        const res = await apiFetch(`/st2138-api/v1/${slot}`);
        if (!res.ok) {
            details.innerHTML = `<div class="device-loading">Error: ${res.status}</div>`;
            return;
        }
        const data = await res.json();
        lastDeviceData = data;
        lastDeviceSlot = slot;
        renderDevice(data);
    } catch (e) {
        console.error('fetchDevice error:', e);
        details.innerHTML = `<div class="device-loading">Error loading device</div>`;
    }
}

// Opens the cached device JSON in the asset-modal popup, re-fetching the
// active slot first so the modal always reflects the latest server state.
async function expandDevice() {
    const slot = lastDeviceSlot ?? 0;
    await fetchDevice(slot);
    if (lastDeviceData === null) return;
    showDeviceModal(lastDeviceData, slot);
}

// Renders the device JSON inside the same modal scaffold used by viewAsset,
// just with the .wide content variant and a syntax-highlighted <pre> body.
function showDeviceModal(deviceData, slot) {
    const existing = document.getElementById('assetModal');
    if (existing) existing.remove();

    const highlighted = syntaxHighlight(deviceData);

    const modal = document.createElement('div');
    modal.id = 'assetModal';
    modal.className = 'asset-modal';
    modal.innerHTML = `
        <div class="asset-modal-content wide">
            <div class="asset-modal-header">
                <span class="asset-modal-title">Device · Slot ${escapeHtml(String(slot))}</span>
                <button class="asset-modal-close" onclick="closeAssetModal()">✕</button>
            </div>
            <div class="asset-modal-body">
                <pre class="json-display">${highlighted}</pre>
            </div>
        </div>
    `;

    modal.addEventListener('click', (e) => {
        if (e.target === modal) closeAssetModal();
    });

    document.body.appendChild(modal);
}

function syntaxHighlight(json) {
    if (typeof json !== 'string') {
        json = JSON.stringify(json, null, 4);
    }
    // Escape HTML
    json = json.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    // Apply syntax highlighting
    return json.replace(/("(\\u[a-zA-Z0-9]{4}|\\[^u]|[^\\"])*"(\s*:)?|\b(true|false|null)\b|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?)/g, function (match) {
        let cls = 'json-number';
        if (/^"/.test(match)) {
            if (/:$/.test(match)) {
                cls = 'json-key';
            } else {
                cls = 'json-string';
            }
        } else if (/true|false/.test(match)) {
            cls = 'json-bool';
        } else if (/null/.test(match)) {
            cls = 'json-null';
        }
        return '<span class="' + cls + '">' + match + '</span>';
    });
}

function renderDevice(device) {
    const details = document.getElementById('deviceDetails');
    const highlighted = syntaxHighlight(device);
    details.innerHTML = `<pre class="json-display">${highlighted}</pre>`;
}

// Helper function to fetch with the auth header and handle 401 errors
async function apiFetch(url, options) {
    const opts = Object.assign({}, options);
    opts.headers = Object.assign({}, opts.headers, { 'Authorization': 'Bearer ' + getToken() });
    const res = await fetch(url, opts);
    if (res.status === 401) {
        console.error('The server rejected this request. Use Authorize to paste a token or sign in.');
    }
    return res;
}

// =====================================================================
// Commands Section
// =====================================================================
async function cmd(name) {
    try {
        const res = await apiFetch(`/st2138-api/v1/0/command/${name}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' }
        });
        const data = await res.json();
        const counter = extractValue(data);
        if (counter !== null) updateCounter(counter);
    } catch (e) { console.error('cmd error:', e); }
}

function updateCounter(value) {
    if (value === undefined || value === null) return;
    const el = document.getElementById('counterValue');
    if (el) el.textContent = value;
}

// Drives the green "running" highlight on the counter and the status badge.
// Backed by the standalone "running" int32 param on slot 0.
function updateRunning(running) {
    if (running === undefined || running === null) return;
    const counterEl = document.getElementById('counterValue');
    const badge = document.getElementById('statusBadge');
    const isRunning = running === 1;
    if (counterEl) counterEl.classList.toggle('running', isRunning);
    if (badge) {
        badge.className = 'status-badge ' + (isRunning ? 'running' : 'stopped');
        badge.textContent = isRunning ? 'Running' : 'Stopped';
    }
}

// =====================================================================
// Parameters Section
// =====================================================================
const paramConfig = [
    { name: 'resolution', type: 'string', slot: 1 },
    { name: 'brightness', type: 'int32', slot: 1, min: 0, max: 100 },
    { name: 'contrast', type: 'int32', slot: 1, min: 0, max: 100 },
    { name: 'saturation', type: 'int32', slot: 1, min: 0, max: 100 },
    { name: 'volume', type: 'int32', slot: 2, min: 0, max: 100 },
    { name: 'muted', type: 'bool', slot: 2 },
    { name: 'device_name', type: 'string', slot: 2 },
];

const editingInputs = new Set();
const suppressedParams = new Map();

function buildParamsUI() {
    const container = document.getElementById('paramsContainer');
    container.innerHTML = paramConfig.map(p => {
        let control;
        if (p.type === 'bool') {
            control = `<label class="toggle-switch">
                <input type="checkbox" id="input-${p.name}"><span class="toggle-slider"></span>
            </label>`;
        } else if (p.type === 'int32' && p.min !== undefined) {
            control = `<input type="range" class="param-slider" id="slider-${p.name}" min="${p.min}" max="${p.max}">
                <input type="number" class="param-input number" id="input-${p.name}" min="${p.min}" max="${p.max}">`;
        } else {
            control = `<input type="text" class="param-input text" id="input-${p.name}">`;
        }
        return `<div class="param-row" id="row-${p.name}">
            <span class="param-name">${p.name}</span>
            <div class="param-control">${control}</div>
        </div>`;
    }).join('');

    // Attach event listeners
    paramConfig.forEach(p => {
        const input = document.getElementById('input-' + p.name);
        const slider = document.getElementById('slider-' + p.name);
        
        input.addEventListener('focus', () => editingInputs.add(p.name));
        input.addEventListener('blur', () => editingInputs.delete(p.name));
        
        if (p.type === 'bool') {
            input.addEventListener('change', () => setParam(p.name, input.checked ? 1 : 0, 'int32'));
        } else if (slider) {
            slider.addEventListener('input', () => {
                editingInputs.add(p.name);
                input.value = slider.value;
            });
            slider.addEventListener('change', () => {
                setParam(p.name, parseInt(slider.value), 'int32');
                setTimeout(() => editingInputs.delete(p.name), 500);
            });
            input.addEventListener('input', () => slider.value = input.value);
            input.addEventListener('change', () => setParam(p.name, parseInt(input.value), 'int32'));
        } else {
            input.addEventListener('change', () => setParam(p.name, input.value, 'string'));
            input.addEventListener('keydown', e => { if (e.key === 'Enter') input.blur(); });
        }
    });
}

function updateParamInput(id, value) {
    if (editingInputs.has(id)) return;
    const expiry = suppressedParams.get(id);
    if (expiry && Date.now() < expiry) return;
    suppressedParams.delete(id);
    const input = document.getElementById('input-' + id);
    if (!input) return;
    if (input.type === 'checkbox') input.checked = value === 1;
    else input.value = value;
    const slider = document.getElementById('slider-' + id);
    if (slider) slider.value = value;
}

async function setParam(name, value, type) {
    suppressedParams.set(name, Date.now() + 500);
    const row = document.getElementById('row-' + name);
    if (row) row.classList.add('saving');
    try {
        const body = type === 'int32' ? { int32_value: value } 
                   : type === 'string' ? { string_value: value } 
                   : value;
        const slot = paramConfig.find(p => p.name === name)?.slot ?? 0;
        const res = await apiFetch(`/st2138-api/v1/${slot}/value/${name}`, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body)
        });
        if (row) {
            row.classList.remove('saving');
            row.classList.add(res.ok || res.status === 204 ? 'saved' : 'error');
            setTimeout(() => row.classList.remove('saved', 'error'), 500);
        }
    } catch (e) {
        console.error('setParam error:', e);
        if (row) { row.classList.remove('saving'); row.classList.add('error'); setTimeout(() => row.classList.remove('error'), 500); }
    }
}

async function poll() {
    const fetches = [];

    // Fetch counter and running flag from slot 0. Both are plain int32 params.
    fetches.push(
        apiFetch('/st2138-api/v1/0/value/counter')
            .then(r => r.json())
            .then(data => {
                const value = extractValue(data);
                if (value !== null) updateCounter(value);
            })
            .catch(e => console.error('poll counter error:', e))
    );
    fetches.push(
        apiFetch('/st2138-api/v1/0/value/running')
            .then(r => r.json())
            .then(data => {
                const value = extractValue(data);
                if (value !== null) updateRunning(value);
            })
            .catch(e => console.error('poll running error:', e))
    );

    // Fetch each param from its proper slot
    for (const p of paramConfig) {
        fetches.push(
            apiFetch(`/st2138-api/v1/${p.slot}/value/${p.name}`)
                .then(r => r.json())
                .then(data => {
                    const val = extractValue(data);
                    if (val !== null) updateParamInput(p.name, val);
                })
                .catch(e => console.error(`poll ${p.name} error:`, e))
        );
    }

    await Promise.all(fetches);
}

// =====================================================================
// SSE Connection (replaces polling)
// =====================================================================
function updateStream(data) {
    if (!data.value) return;

    const oid = data.value.oid;
    const val = extractValue(data.value.value);

    if (val == null) return;

    if (oid == 'counter')
    {
        updateCounter(val);
    }
    else if (oid == 'running')
    {
        updateRunning(val);
    }
    else
    {
        updateParamInput(oid, val);
    }
}

function handleSSEFrame(frame) {
    const data = frame.split('\n')
        .filter(line => line.startsWith('data:'))
        .map(line => line.slice(5).trimStart())
        .join('\n');
    if (!data) return;
    try {
        updateStream(JSON.parse(data));
    } catch (e) {
        console.error('SSE parse error:', e);
    }
}

async function connectSSE() {


    try {
        const res = await apiFetch('/st2138-api/v1/connect', {
            headers: { Accept: 'text/event-stream' },
        });

        if (!res.ok || !res.body) {
            setTimeout(() => connectSSE(), 2000);
            return;
        }
        const reader = res.body.getReader();
        const decoder = new TextDecoder();
        let buffer = '';
        while (true) {
            const { value, done } = await reader.read();

            if (done) break;

            buffer += decoder.decode(value, {stream: true});

            // SSE frame
            let splitAt;
            while ((splitAt = buffer.indexOf('\n\n')) !== -1) {
                handleSSEFrame(buffer.slice(0, splitAt));
                buffer = buffer.slice(splitAt + 2);
            }
        }
    } catch (e) {
        console.warn('SSE connection lost, will retry');
        setTimeout(() => connectSSE(), 2000);
    }
}

// =====================================================================
// Assets Section
// =====================================================================
async function fetchAssets() {
    const container = document.getElementById('assetsContainer');
    try {
        const res = await apiFetch('/assets-list');
        if (!res.ok) {
            container.innerHTML = '<div class="asset-error">Failed to load assets</div>';
            return;
        }
        const assets = await res.json();
        renderAssets(assets);
    } catch (e) {
        console.error('fetchAssets error:', e);
        container.innerHTML = '<div class="asset-error">Error loading assets</div>';
    }
}

function formatSize(bytes) {
    if (bytes < 1024) return bytes + ' B';
    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
    return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
}

function renderAssets(assets) {
    const container = document.getElementById('assetsContainer');
    if (!assets || assets.length === 0) {
        container.innerHTML = '<div class="asset-empty">No assets found</div>';
        return;
    }
    
    container.innerHTML = assets.map(asset => `
        <div class="asset-item">
            <div class="asset-info">
                <span class="asset-name">${asset.file_name}</span>
                <span class="asset-meta">${asset.content_type} · ${formatSize(asset.size)}</span>
            </div>
            <button class="asset-btn" onclick="viewAsset('${asset.id}', '${asset.content_type}', '${asset.file_name}')" title="View">👁</button>
        </div>
    `).join('');
}

// Decode base64 payload from asset response
function decodeAssetPayload(base64) {
    const binary = atob(base64);
    const bytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) {
        bytes[i] = binary.charCodeAt(i);
    }
    return bytes;
}

// Fetch and decode asset from API
async function fetchAssetData(assetId) {
    const res = await apiFetch(`/st2138-api/v1/0/asset/${assetId}`);
    if (!res.ok) throw new Error(`Failed to fetch asset: ${res.status}`);
    const data = await res.json();
    
    const base64 = data.payload?.payload;
    if (!base64) throw new Error('No payload in asset response');
    
    return decodeAssetPayload(base64);
}

// View asset in modal or new tab
async function viewAsset(assetId, contentType, fileName) {
    try {
        const bytes = await fetchAssetData(assetId);
        const blob = new Blob([bytes], { type: contentType });
        
        if (contentType.startsWith('image/')) {
            // Show image in modal
            showAssetModal(URL.createObjectURL(blob), 'image', fileName);
        } else if (contentType.startsWith('text/')) {
            // Show text content in modal
            const text = new TextDecoder().decode(bytes);
            showAssetModal(text, 'text', fileName);
        } else {
            // For other types, open in new tab or download
            const url = URL.createObjectURL(blob);
            window.open(url, '_blank');
        }
    } catch (e) {
        console.error('viewAsset error:', e);
        alert('Failed to view asset: ' + e.message);
    }
}

// Show asset content in a modal
function showAssetModal(content, type, fileName) {
    // Remove existing modal if any
    const existing = document.getElementById('assetModal');
    if (existing) existing.remove();
    
    const modal = document.createElement('div');
    modal.id = 'assetModal';
    modal.className = 'asset-modal';
    
    let contentHtml;
    if (type === 'image') {
        contentHtml = `<img src="${content}" alt="${fileName}" style="max-width: 100%; max-height: 70vh;">`;
    } else {
        contentHtml = `<pre style="text-align: left; white-space: pre-wrap; word-wrap: break-word; max-height: 70vh; overflow: auto;">${escapeHtml(content)}</pre>`;
    }
    
    modal.innerHTML = `
        <div class="asset-modal-content">
            <div class="asset-modal-header">
                <span class="asset-modal-title">${escapeHtml(fileName)}</span>
                <button class="asset-modal-close" onclick="closeAssetModal()">✕</button>
            </div>
            <div class="asset-modal-body">
                ${contentHtml}
            </div>
        </div>
    `;
    
    modal.addEventListener('click', (e) => {
        if (e.target === modal) closeAssetModal();
    });
    
    document.body.appendChild(modal);
}

function closeAssetModal() {
    const modal = document.getElementById('assetModal');
    if (modal) {
        // Revoke any blob URLs for images
        const img = modal.querySelector('img');
        if (img && img.src.startsWith('blob:')) {
            URL.revokeObjectURL(img.src);
        }
        modal.remove();
    }
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

// =====================================================================
// Authorization
// =====================================================================

let demoConfig = {
    authzEnabled: false
}

async function loadDemoConfig() {
    try {
        const res = await apiFetch('/demo-config');
        if (!res.ok) return;
        const data = await res.json();
        demoConfigs = Object.assign(demoConfig, data);
    } catch (e) {
        console.error('demo-config error:', e);
    }
}

// Session storage for the token
TOKEN_STORAGE_KEY = 'st2138-token';

function getRawToken(token) {
    token = (token || '').trim();

    // Remove the Bearer prefix if it exists
    if (token.toLowerCase().startsWith('bearer ')) {
        token = token.replace('bearer ', '').trim();
    }
    return token;
}

function getToken() {
    return sessionStorage.getItem(TOKEN_STORAGE_KEY) || '';
}

function setToken(token) {
    token = getRawToken(token);
    if (token) sessionStorage.setItem(TOKEN_STORAGE_KEY, token);
    else sessionStorage.removeItem(TOKEN_STORAGE_KEY);
    return token;
}

function openAuthModal() {
    const modal = document.getElementById('authModal');
    const input = document.getElementById('tokenInput');
    
    modal.classList.add('open');
    input.focus();
}

function closeAuthModal() {
    document.getElementById('authModal').classList.remove('open');
}

function selectedScope() {
    const read = new Set();
    const write = new Set();

    scopeCheckBoxList = Array.from(document.querySelectorAll('#authModal input[type="checkbox"][data-scope]'));

    scopeCheckBoxList.forEach(box => {
        if (!box.checked) return;
        if (box.dataset.access === 'write') write.add(box.dataset.scope);
        else read.add(box.dataset.scope);
    });

    write.forEach(scope => read.add(scope)); // A write scope impled a read scope aswell

    // Build the scope string with the read and write scopes and the roles
    const names = [];
    ['st2138:mon', 'st2138:op', 'st2138:cfg', 'st2138:adm'].forEach(scope => {
        if (read.has(scope)) names.push(scope);
        if (write.has(scope)) names.push(scope + ':w');
    });

    return names.join(' ');
}

function bytesToBase64Url(bytes) {
    let binary = '';
    for (let i = 0; i < bytes.length; i++) binary += String.fromCharCode(bytes[i]);
    return btoa(binary).replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/g, '');
}

function refreshAfterAuthChange() {
    poll();
    connectSSE();
    fetchDevice(lastDeviceSlot ?? 0);
    fetchAssets();
}

// Generate unsigned token for the demo with the selected scopes
function buildDemoToken() {

    const now = Math.floor(Date.now() / 1000);

    const claims = {
        sub: 'demo-user',
        scope: selectedScope(),
        iat: now - 30,
        exp: now + 3600,
    };

    const headerJSON = JSON.stringify({ alg: 'ES256', typ: 'JWT' });
    const payloadJSON = JSON.stringify(claims);

    const headerBytes = new TextEncoder().encode(headerJSON);
    const payloadBytes = new TextEncoder().encode(payloadJSON);
    const signatureBytes = new TextEncoder().encode('demo');

    const header = bytesToBase64Url(headerBytes);
    const payload = bytesToBase64Url(payloadBytes);
    const signature = bytesToBase64Url(signatureBytes);

    return header + '.' + payload + '.' + signature;
}

function bindAuthUI() {
    document.getElementById('authorizeBtn').addEventListener('click', openAuthModal);
    document.getElementById('authModalClose').addEventListener('click', closeAuthModal);
    document.getElementById('authModal').addEventListener('click', (e) => {
        if (e.target.id === 'authModal') closeAuthModal();
    });


    document.getElementById('generateTokenBtn').addEventListener('click', () => {

        console.log('generateTokenBtn clicked');
        const token = buildDemoToken();
        document.getElementById('tokenInput').value = token;
        
        setToken(token);
        
        closeAuthModal();

        refreshAfterAuthChange();
    });
}

// =====================================================================
// Initialization
// =====================================================================
document.addEventListener('DOMContentLoaded', async () => {
    bindAuthUI();
    buildParamsUI();

    await loadDemoConfig();
    
    poll();
    connectSSE();
    fetchDevice(0);
    fetchAssets();
});
