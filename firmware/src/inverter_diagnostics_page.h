#pragma once
#include <Arduino.h>

const char INVERTER_DIAGNOSTICS_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>NEXT Inverter — Live Parameter Diagnostics & Settings Manager</title>
<style>
  :root {
    --bg-main: #f1f5f9;
    --card-bg: #ffffff;
    --border: #e2e8f0;
    --text-primary: #0f172a;
    --text-secondary: #64748b;
    --brand-blue: #2563eb;
    --brand-green: #059669;
    --brand-orange: #d97706;
    --brand-red: #dc2626;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
  body { background: var(--bg-main); color: var(--text-primary); padding: 14px; min-height: 100vh; }
  .container { max-width: 900px; margin: 0 auto; display: flex; flex-direction: column; gap: 14px; }
  
  /* Header */
  .top-nav {
    background: #0f172a;
    color: #ffffff;
    padding: 14px 18px;
    border-radius: 14px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 10px;
    box-shadow: 0 4px 12px rgba(15, 23, 42, 0.15);
  }
  .top-nav h1 { font-size: 1.15rem; font-weight: 800; display: flex; align-items: center; gap: 8px; }
  .top-nav .sub { font-size: 0.72rem; color: #94a3b8; margin-top: 2px; }
  .nav-btns { display: flex; gap: 8px; align-items: center; }
  .btn-nav {
    background: rgba(255, 255, 255, 0.12);
    color: #ffffff;
    border: 1px solid rgba(255, 255, 255, 0.2);
    padding: 7px 14px;
    border-radius: 8px;
    font-size: 0.78rem;
    font-weight: 700;
    cursor: pointer;
    text-decoration: none;
    display: inline-flex;
    align-items: center;
    gap: 5px;
    transition: 0.15s ease;
  }
  .btn-nav:hover { background: rgba(255, 255, 255, 0.22); }
  .btn-refresh-all {
    background: #2563eb;
    border-color: #3b82f6;
  }
  .btn-refresh-all:hover { background: #1d4ed8; }

  /* Notification Banner */
  .status-banner {
    padding: 12px 16px;
    border-radius: 10px;
    font-size: 0.8rem;
    font-weight: 600;
    display: none;
    line-height: 1.4;
  }
  .status-banner.success { background: #ecfdf5; border: 1px solid #a7f3d0; color: #065f46; display: block; }
  .status-banner.error { background: #fef2f2; border: 1px solid #fecaca; color: #991b1b; display: block; }
  .status-banner.running { background: #eff6ff; border: 1px solid #bfdbfe; color: #1e40af; display: block; }

  /* Raw Diagnostic Terminal */
  .diag-card {
    background: #1e293b;
    color: #e2e8f0;
    padding: 14px 16px;
    border-radius: 12px;
    box-shadow: 0 2px 8px rgba(0,0,0,0.06);
  }
  .diag-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 8px;
    font-size: 0.78rem;
    font-weight: 700;
    color: #94a3b8;
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }
  .raw-box {
    background: #0f172a;
    padding: 8px 12px;
    border-radius: 6px;
    font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace;
    font-size: 0.72rem;
    color: #38bdf8;
    word-break: break-all;
    margin-bottom: 6px;
    line-height: 1.4;
    border: 1px solid #334155;
  }
  .raw-box b { color: #f59e0b; }

  /* Category Section */
  .section-card {
    background: var(--card-bg);
    border-radius: 14px;
    padding: 16px;
    box-shadow: 0 2px 8px rgba(0,0,0,0.04);
    border: 1px solid var(--border);
  }
  .section-title {
    font-size: 0.95rem;
    font-weight: 800;
    color: #0f172a;
    margin-bottom: 12px;
    display: flex;
    align-items: center;
    gap: 8px;
    border-bottom: 1px solid #f1f5f9;
    padding-bottom: 8px;
  }

  /* Settings Grid */
  .param-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(270px, 1fr));
    gap: 12px;
  }
  .param-item {
    background: #f8fafc;
    border: 1px solid #e2e8f0;
    border-radius: 10px;
    padding: 12px;
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    transition: 0.2s ease;
  }
  .param-item.updated {
    border-color: #10b981;
    background: #ecfdf5;
    box-shadow: 0 0 0 2px rgba(16, 185, 129, 0.2);
  }
  .param-prog {
    font-size: 0.68rem;
    font-weight: 800;
    color: #64748b;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 4px;
  }
  .badge-val {
    background: #e2e8f0;
    color: #0f172a;
    font-size: 0.68rem;
    font-weight: 700;
    padding: 2px 6px;
    border-radius: 4px;
    font-family: monospace;
  }
  .badge-val.verified { background: #dcfce7; color: #15803d; }
  .param-name {
    font-size: 0.84rem;
    font-weight: 700;
    color: #1e293b;
    margin-bottom: 6px;
  }
  .param-desc {
    font-size: 0.7rem;
    color: #64748b;
    margin-bottom: 10px;
    line-height: 1.35;
  }
  .param-control {
    display: flex;
    gap: 6px;
    align-items: center;
    margin-top: auto;
  }
  .param-control select, .param-control input {
    flex: 1;
    padding: 7px 10px;
    border: 1px solid #cbd5e1;
    border-radius: 6px;
    font-size: 0.78rem;
    font-weight: 600;
    background: #ffffff;
    color: #0f172a;
    outline: none;
  }
  .param-control select:focus, .param-control input:focus {
    border-color: #2563eb;
    box-shadow: 0 0 0 2px rgba(37, 99, 235, 0.15);
  }
  .btn-apply {
    background: #2563eb;
    color: #ffffff;
    border: none;
    padding: 7px 12px;
    border-radius: 6px;
    font-size: 0.75rem;
    font-weight: 700;
    cursor: pointer;
    white-space: nowrap;
    transition: 0.15s ease;
  }
  .btn-apply:hover { background: #1d4ed8; }
  .btn-apply:disabled { opacity: 0.5; cursor: not-allowed; }

  /* Direct RS232 Console Card */
  .console-row {
    display: flex;
    gap: 8px;
    margin-top: 8px;
  }
  .console-input {
    flex: 1;
    padding: 8px 12px;
    border: 1px solid #334155;
    background: #0f172a;
    color: #38bdf8;
    font-family: monospace;
    font-size: 0.85rem;
    border-radius: 6px;
    outline: none;
  }
  .btn-send-raw {
    background: #059669;
    color: #fff;
    border: none;
    padding: 8px 16px;
    border-radius: 6px;
    font-size: 0.78rem;
    font-weight: 700;
    cursor: pointer;
  }
  .btn-send-raw:hover { background: #047857; }
</style>
</head>
<body>

<div class="container">

  <!-- Top Navigation & Title Bar -->
  <div class="top-nav">
    <div>
      <h1>⚙️ Inverter Parameter Diagnostics & Live Settings</h1>
      <div class="sub">NEXT NM-ECO-6.2KW PLUS • Live RS-232 Configuration (Voltronic PI30 Protocol)</div>
    </div>
    <div class="nav-btns">
      <a href="/" class="btn-nav">⬅️ Back to Dashboard</a>
      <button class="btn-nav btn-refresh-all" id="btnRefreshAll" onclick="fetchAllSettings()">🔄 Refresh Inverter</button>
    </div>
  </div>

  <!-- Status / Verification Feedback Banner -->
  <div id="statusBanner" class="status-banner"></div>

  <!-- Diagnostic Raw Strings Strip -->
  <div class="diag-card">
    <div class="diag-header">
      <span>📡 Live RS-232 Serial Packets</span>
      <span id="lastRefreshedTime" style="color:#64748b; font-size:0.7rem;">Waiting for query...</span>
    </div>
    <div class="raw-box"><b>QPIRI:</b> <span id="rawQpiri">(Click Refresh to query...)</span></div>
    <div class="raw-box"><b>QFLAG:</b> <span id="rawQflag">(Click Refresh to query...)</span></div>
    <div class="raw-box"><b>QMOD:</b> <span id="rawQmod">(Click Refresh to query...)</span></div>

    <!-- Direct Serial Command Tester -->
    <div class="console-row">
      <input type="text" id="rawCmdInput" class="console-input" placeholder="Enter raw command (e.g. PCP01, PCP02, PCP03, POP02, QPIRI)..." onkeydown="if(event.key==='Enter') sendDirectCmd()">
      <button class="btn-send-raw" onclick="sendDirectCmd()">Send Direct</button>
    </div>
  </div>

  <!-- Section 1: Power & Charger Priorities -->
  <div class="section-card">
    <div class="section-title">⚡ 1. Power Output & Battery Charger Priorities</div>
    <div class="param-grid">

      <!-- Program 01: Output Source Priority -->
      <div class="param-item" id="card_prog01">
        <div>
          <div class="param-prog">
            <span>Program 01</span>
            <span class="badge-val verified" id="val_prog01">Loading...</span>
          </div>
          <div class="param-name">Output Source Priority</div>
          <div class="param-desc">Determines whether house loads are powered by Solar/Battery first (SBU) or Utility Grid first (USB).</div>
        </div>
        <div class="param-control">
          <select id="sel_prog01">
            <option value="POP02">SBU: Solar First, Battery Second, Grid Last</option>
            <option value="POP01">SUB: Solar First, Utility Blending Assistance</option>
            <option value="POP00">USB: Utility Grid First (Grid powers load)</option>
          </select>
          <button class="btn-apply" id="btn_prog01" onclick="applySingleParam('sel_prog01', 'Output Source Priority')">Apply</button>
        </div>
      </div>

      <!-- Program 16: Charger Source Priority -->
      <div class="param-item" id="card_prog16">
        <div>
          <div class="param-prog">
            <span>Program 16</span>
            <span class="badge-val verified" id="val_prog16">Loading...</span>
          </div>
          <div class="param-name">Charger Source Priority</div>
          <div class="param-desc">Controls which source charges your battery. (Calibrated: PCP02 = OSO, PCP01 = SNU, PCP03 = CSO on LCD).</div>
        </div>
        <div class="param-control">
          <select id="sel_prog16">
            <option value="PCP02">PCP02: Only Solar Charging (OSO on Inverter LCD)</option>
            <option value="PCP01">PCP01: Solar & Utility Charging (SNU on Inverter LCD)</option>
            <option value="PCP03">PCP03: Solar Energy First (CSO on Inverter LCD)</option>
            <option value="PCP00">PCP00: Utility First Charging</option>
          </select>
          <button class="btn-apply" id="btn_prog16" onclick="applySingleParam('sel_prog16', 'Charger Source Priority')">Apply</button>
        </div>
      </div>

      <!-- Program 23: Overload Bypass -->
      <div class="param-item" id="card_prog23">
        <div>
          <div class="param-prog">
            <span>Program 23</span>
            <span class="badge-val verified" id="val_prog23">Loading...</span>
          </div>
          <div class="param-name">Overload Bypass to Grid</div>
          <div class="param-desc">If enabled, transfers load to utility if inverter exceeds 100% capacity in battery mode.</div>
        </div>
        <div class="param-control">
          <select id="sel_prog23">
            <option value="PEb">PEb: Enable Overload Bypass (Transfer to Grid on overload)</option>
            <option value="PDb">PDb: Disable Overload Bypass</option>
          </select>
          <button class="btn-apply" id="btn_prog23" onclick="applySingleParam('sel_prog23', 'Overload Bypass')">Apply</button>
        </div>
      </div>

    </div>
  </div>

  <!-- Section 2: Safe Charging Current Limits -->
  <div class="section-card">
    <div class="section-title">🔌 2. Total & Utility Charging Amperages (Max 30A Safety Limit)</div>
    <div class="param-grid">

      <!-- Program 02: Maximum Charging Current -->
      <div class="param-item" id="card_prog02">
        <div>
          <div class="param-prog">
            <span>Program 02</span>
            <span class="badge-val verified" id="val_prog02">Loading...</span>
          </div>
          <div class="param-name">Max Total Charging Current</div>
          <div class="param-desc">Combined charging limit (Solar MPPT + Utility Grid). Capped at 30A max for 50Ah Leaf battery safety.</div>
        </div>
        <div class="param-control">
          <select id="sel_prog02">
            <option value="MNCHGC010">10 Amps (~500 Watts - Ultra gentle trickle)</option>
            <option value="MNCHGC020">20 Amps (~1000 Watts - Standard safe rate)</option>
            <option value="MNCHGC030">30 Amps (~1500 Watts - Max Safe Rate for 50Ah)</option>
          </select>
          <button class="btn-apply" id="btn_prog02" onclick="applySingleParam('sel_prog02', 'Max Total Charge Current')">Apply</button>
        </div>
      </div>

      <!-- Program 11: Maximum Utility Charging Current -->
      <div class="param-item" id="card_prog11">
        <div>
          <div class="param-prog">
            <span>Program 11</span>
            <span class="badge-val verified" id="val_prog11">Loading...</span>
          </div>
          <div class="param-name">Max Utility Grid Charge Current</div>
          <div class="param-desc">Limits AC charging current from CEB utility grid. Set to 2A for near-zero grid battery charging.</div>
        </div>
        <div class="param-control">
          <select id="sel_prog11">
            <option value="MUCHGC002">2 Amps (Zero / Minimal Trickle)</option>
            <option value="MUCHGC010">10 Amps (~500 Watts - Gentle night charge)</option>
            <option value="MUCHGC020">20 Amps (~1000 Watts - Standard grid charge)</option>
            <option value="MUCHGC030">30 Amps (~1500 Watts - Max Safe Grid Charge)</option>
          </select>
          <button class="btn-apply" id="btn_prog11" onclick="applySingleParam('sel_prog11', 'Max Utility Charge Current')">Apply</button>
        </div>
      </div>

    </div>
  </div>

  <!-- Section 3: Grid Transfer Voltages -->
  <div class="section-card">
    <div class="section-title">🔋 3. Grid Transfer Voltages (SBU / SUB Cycling Thresholds)</div>
    <div class="param-grid">

      <!-- Program 12: Back to Grid in SBU / SUB -->
      <div class="param-item" id="card_prog12">
        <div>
          <div class="param-prog">
            <span>Program 12</span>
            <span class="badge-val verified" id="val_prog12">Loading...</span>
          </div>
          <div class="param-name">Back to Utility Grid Voltage (V)</div>
          <div class="param-desc">In SBU mode OR Solar First (SUB) mode, voltage threshold where inverter transfers load to Grid. Inverter selectable range is 42.0V to 51.0V (Recommended for Leaf: 47.6V–48.0V).</div>
        </div>
        <div class="param-control">
          <input type="number" step="0.1" min="42.0" max="51.0" id="in_prog12" placeholder="42.0 - 51.0">
          <button class="btn-apply" id="btn_prog12" onclick="applyVoltParam('in_prog12', 'PBCV', 'Back to Grid Voltage', 42.0, 51.0)">Apply</button>
        </div>
      </div>

      <!-- Program 13: Back to Battery in SBU / SUB -->
      <div class="param-item" id="card_prog13">
        <div>
          <div class="param-prog">
            <span>Program 13</span>
            <span class="badge-val verified" id="val_prog13">Loading...</span>
          </div>
          <div class="param-name">Back to Battery Discharge (V)</div>
          <div class="param-desc">In SBU mode OR Solar First (SUB) mode, threshold where inverter switches from Grid back to Battery. Inverter selectable range is 48.0V to 58.0V (Recommended for Leaf: 51.0V–54.0V).</div>
        </div>
        <div class="param-control">
          <input type="number" step="0.1" min="48.0" max="58.0" id="in_prog13" placeholder="48.0 - 58.0">
          <button class="btn-apply" id="btn_prog13" onclick="applyVoltParam('in_prog13', 'PBDV', 'Back to Battery Voltage', 48.0, 58.0)">Apply</button>
        </div>
      </div>

    </div>
  </div>

</div>

<script>
let liveSettingsData = null;

async function fetchAllSettings() {
  const btn = document.getElementById('btnRefreshAll');
  if (btn) {
    btn.disabled = true;
    btn.textContent = '⏳ Querying...';
  }
  showBanner('⏳ Querying all parameters from Inverter EEPROM via RS-232...', 'running');

  try {
    const res = await fetch('/api/inverter_settings');
    if (!res.ok) throw new Error('HTTP ' + res.status);
    const d = await res.json();
    liveSettingsData = d;
    renderSettingsView(d);
    showBanner(`✅ <b>Live settings refreshed!</b> Inverter returned valid parameters via QPIRI & QFLAG.`, 'success');
  } catch(e) {
    showBanner(`❌ <b>Error:</b> Could not query settings from ESP32. (${e.message})`, 'error');
  } finally {
    if (btn) {
      btn.disabled = false;
      btn.textContent = '🔄 Refresh Inverter';
    }
  }
}

function renderSettingsView(d) {
  document.getElementById('lastRefreshedTime').textContent = 'Last Query: ' + new Date().toLocaleTimeString();
  document.getElementById('rawQpiri').textContent = d.raw_qpiri || '(No QPIRI received)';
  document.getElementById('rawQflag').textContent = d.raw_qflag || '(No QFLAG received)';
  document.getElementById('rawQmod').textContent = (d.raw_qmod || '') + (d.mode ? ` (${d.mode})` : '');

  const s = d.settings || {};

  // Prog 01: Output Priority
  if (s.out_priority) {
    const el = document.getElementById('val_prog01');
    if (el) el.textContent = s.out_priority.code + ': ' + s.out_priority.name;
    const sel = document.getElementById('sel_prog01');
    if (sel) sel.value = s.out_priority.cmd;
  }

  // Prog 16: Charger Priority
  if (s.chg_priority) {
    const el = document.getElementById('val_prog16');
    if (el) el.textContent = `Token[17]=${s.chg_priority.code} (${s.chg_priority.name})`;
    const sel = document.getElementById('sel_prog16');
    if (sel) sel.value = s.chg_priority.cmd;
  }

  // Prog 23: Overload Bypass
  if (s.overload_bypass) {
    const el = document.getElementById('val_prog23');
    if (el) el.textContent = s.overload_bypass.enabled ? 'Enabled (PEb)' : 'Disabled (PDb)';
    const sel = document.getElementById('sel_prog23');
    if (sel) sel.value = s.overload_bypass.enabled ? 'PEb' : 'PDb';
  }

  // Prog 02: Max Total Charge Current
  if (s.max_chg_curr) {
    const el = document.getElementById('val_prog02');
    if (el) el.textContent = s.max_chg_curr.value + ' A';
    const sel = document.getElementById('sel_prog02');
    if (sel) sel.value = s.max_chg_curr.cmd;
  }

  // Prog 11: Max Utility Charge Current
  if (s.max_util_curr) {
    const el = document.getElementById('val_prog11');
    if (el) el.textContent = s.max_util_curr.value + ' A';
    const sel = document.getElementById('sel_prog11');
    if (sel) sel.value = s.max_util_curr.cmd;
  }

  // Prog 12: Back to Grid
  if (s.back_to_grid) {
    const el = document.getElementById('val_prog12');
    if (el) el.textContent = s.back_to_grid.value.toFixed(1) + ' V';
    const inp = document.getElementById('in_prog12');
    if (inp) inp.value = s.back_to_grid.value.toFixed(1);
  }

  // Prog 13: Back to Battery
  if (s.back_to_bat) {
    const el = document.getElementById('val_prog13');
    if (el) el.textContent = s.back_to_bat.value.toFixed(1) + ' V';
    const inp = document.getElementById('in_prog13');
    if (inp) inp.value = s.back_to_bat.value.toFixed(1);
  }
}

async function applySingleParam(selectId, paramName) {
  const sel = document.getElementById(selectId);
  const cmd = sel.value;
  if (!cmd) return;

  const btn = sel.nextElementSibling;
  if (btn) {
    btn.disabled = true;
    btn.textContent = 'Saving...';
  }

  showBanner(`⏳ Sending command <code>${cmd}</code> to Inverter for ${paramName}...`, 'running');

  try {
    const res = await fetch('/api/set_inverter_param', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'cmd=' + encodeURIComponent(cmd)
    });
    if (!res.ok) throw new Error('HTTP ' + res.status);
    const result = await res.json();
    
    if (result.ack) {
      showBanner(`✅ <b>${paramName} updated!</b> Inverter confirmed with ACK. Re-querying verified EEPROM value...`, 'success');
      // Highlight the updated card
      const card = sel.closest('.param-item');
      if (card) {
        card.classList.add('updated');
        setTimeout(() => card.classList.remove('updated'), 4000);
      }
      // Re-query all settings to verify new state from physical inverter!
      setTimeout(fetchAllSettings, 700);
    } else {
      showBanner(`❌ <b>Inverter rejected command:</b> <code>${cmd}</code> (Inverter response: <code>${result.resp || 'No ACK'}</code>)`, 'error');
    }
  } catch(e) {
    showBanner(`❌ <b>Network error:</b> Could not send command. (${e.message})`, 'error');
  } finally {
    if (btn) {
      btn.disabled = false;
      btn.textContent = 'Apply';
    }
  }
}

async function applyVoltParam(inputId, prefix, paramName, minV, maxV) {
  const val = parseFloat(document.getElementById(inputId).value);
  if (isNaN(val) || val < minV || val > maxV) {
    alert(`Please enter a valid voltage between ${minV}V and ${maxV}V.`);
    return;
  }

  const cmd = prefix + val.toFixed(1);
  const btn = document.getElementById(inputId).nextElementSibling;
  if (btn) {
    btn.disabled = true;
    btn.textContent = 'Saving...';
  }

  showBanner(`⏳ Sending voltage command <code>${cmd}</code> to Inverter for ${paramName}...`, 'running');

  try {
    const res = await fetch('/api/set_inverter_param', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'cmd=' + encodeURIComponent(cmd)
    });
    const result = await res.json();
    if (result.ack) {
      showBanner(`✅ <b>${paramName} set to ${val.toFixed(1)}V!</b> Re-querying verified EEPROM value...`, 'success');
      setTimeout(fetchAllSettings, 700);
    } else {
      showBanner(`❌ <b>Inverter rejected voltage:</b> <code>${cmd}</code> (${result.resp})`, 'error');
    }
  } catch(e) {
    showBanner(`❌ <b>Network error:</b> Could not send voltage command.`, 'error');
  } finally {
    if (btn) {
      btn.disabled = false;
      btn.textContent = 'Apply';
    }
  }
}

async function sendDirectCmd() {
  const input = document.getElementById('rawCmdInput');
  const cmd = input.value.trim();
  if (!cmd) return;

  showBanner(`⏳ Sending direct RS-232 command: <code>${cmd}</code>...`, 'running');

  try {
    const res = await fetch('/api/send_cmd', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'cmd=' + encodeURIComponent(cmd)
    });
    const text = await res.text();
    showBanner(`📡 <b>Command:</b> <code>${cmd}</code> ➔ <b>Inverter Response:</b> <code>${text}</code>`, 'success');
    setTimeout(fetchAllSettings, 1000);
  } catch(e) {
    showBanner(`❌ Error sending direct command.`, 'error');
  }
}

function showBanner(html, type) {
  const b = document.getElementById('statusBanner');
  b.className = `status-banner ${type}`;
  b.innerHTML = html;
}

// Auto-load settings on initial page open
window.addEventListener('DOMContentLoaded', fetchAllSettings);
</script>

</body>
</html>
)rawliteral";
