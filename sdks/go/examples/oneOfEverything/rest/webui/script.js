let currentSlot = 0;

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
async function selectSlot(slot) {
    currentSlot = slot;
    
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
        const res = await fetch(`/st2138-api/v1/${slot}`);
        if (!res.ok) {
            details.innerHTML = `<div class="device-loading">Error: ${res.status}</div>`;
            return;
        }
        const data = await res.json();
        renderDevice(data);
    } catch (e) {
        console.error('fetchDevice error:', e);
        details.innerHTML = `<div class="device-loading">Error loading device</div>`;
    }
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

// =====================================================================
// Commands Section
// =====================================================================
async function cmd(name) {
    try {
        const res = await fetch(`/st2138-api/v1/0/command/${name}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' }
        });
        const data = await res.json();
        const fields = data.struct_value?.fields;
        if (fields) {
            const counter = extractValue(fields.counter);
            const running = extractValue(fields.running);
            updateCounter(counter, running);
        }
    } catch (e) { console.error('cmd error:', e); }
}

function updateCounter(value, running) {
    const el = document.getElementById('counterValue');
    const badge = document.getElementById('statusBadge');
    el.textContent = value ?? 0;
    // running is int32: 0 = false, 1 = true
    if (running === 1) {
        el.classList.add('running');
        badge.className = 'status-badge running';
        badge.textContent = 'Running';
    } else {
        el.classList.remove('running');
        badge.className = 'status-badge stopped';
        badge.textContent = 'Stopped';
    }
}

// =====================================================================
// Parameters Section
// =====================================================================
const paramConfig = [
    { name: 'resolution', type: 'string' },
    { name: 'brightness', type: 'int32', min: 0, max: 100 },
    { name: 'contrast', type: 'int32', min: 0, max: 100 },
    { name: 'saturation', type: 'int32', min: 0, max: 100 },
    { name: 'volume', type: 'int32', min: 0, max: 100 },
    { name: 'muted', type: 'bool' },
    { name: 'device_name', type: 'string' },
];

const editingInputs = new Set();

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
            slider.addEventListener('input', () => input.value = slider.value);
            slider.addEventListener('change', () => setParam(p.name, parseInt(slider.value), 'int32'));
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
    const input = document.getElementById('input-' + id);
    if (!input) return;
    if (input.type === 'checkbox') input.checked = value === 1;
    else input.value = value;
    const slider = document.getElementById('slider-' + id);
    if (slider) slider.value = value;
}

async function setParam(name, value, type) {
    const row = document.getElementById('row-' + name);
    if (row) row.classList.add('saving');
    try {
        const body = type === 'int32' ? { int32_value: value } 
                   : type === 'string' ? { string_value: value } 
                   : value;
        const res = await fetch(`/st2138-api/v1/0/value/${name}`, {
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
    try {
        const res = await fetch('/st2138-api/v1/0/value/all');
        const data = await res.json();
        const fields = data.struct_value?.fields;
        if (fields) {
            // Update counter
            const counter = extractValue(fields.counter);
            const running = extractValue(fields.counter_running);
            updateCounter(counter, running);
            
            // Update param inputs
            for (const [key, protoVal] of Object.entries(fields)) {
                if (key !== 'counter' && key !== 'counter_running') {
                    updateParamInput(key, extractValue(protoVal));
                }
            }
        }
    } catch (e) { console.error('poll error:', e); }
}

// =====================================================================
// Initialization
// =====================================================================
document.addEventListener('DOMContentLoaded', () => {
    buildParamsUI();
    setInterval(poll, 500);
    poll();
    fetchDevice(0);
});
