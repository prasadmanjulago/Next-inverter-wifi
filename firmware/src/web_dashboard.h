#pragma once
#include <Arduino.h>

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Next 6.2kW Inverter — Solar Dashboard</title>
<style>
  :root {
    --bg-top: #cfe4fc;
    --bg-mid: #e9f2fd;
    --bg-bot: #f8fafc;
    --text-main: #1e293b;
    --text-muted: #64748b;
    --solar: #f59e0b;
    --grid: #2563eb;
    --battery: #10b981;
    --load: #3b82f6;
    --inverter: #f97316;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    background: #0f172a;
    min-height: 100vh;
    display: flex;
    justify-content: center;
    align-items: center;
    padding: 12px;
  }
  .app-frame {
    width: 100%;
    max-width: 420px;
    background: linear-gradient(180deg, var(--bg-top) 0%, var(--bg-mid) 45%, var(--bg-bot) 100%);
    border-radius: 28px;
    box-shadow: 0 25px 50px -12px rgba(0,0,0,0.5);
    overflow: hidden;
    position: relative;
    padding: 18px 16px 16px 16px;
  }

  /* OTA Banner */
  .ota-banner {
    display: none;
    background: linear-gradient(135deg, #059669, #10b981);
    color: white;
    padding: 8px 12px;
    border-radius: 12px;
    margin-bottom: 10px;
    font-size: 0.74rem;
    font-weight: 600;
    box-shadow: 0 4px 12px rgba(16,185,129,0.3);
    justify-content: space-between;
    align-items: center;
  }
  .btn-ota {
    background: #ffffff;
    color: #065f46;
    border: none;
    padding: 4px 10px;
    border-radius: 6px;
    font-weight: 700;
    cursor: pointer;
    font-size: 0.72rem;
  }

  /* Header */
  .header-bar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 8px;
    padding: 0 8px;
  }
  .header-bar h2 {
    font-size: 1rem;
    font-weight: 700;
    color: var(--text-main);
  }
  .conn-pill {
    font-size: 0.68rem;
    font-weight: 700;
    padding: 3px 8px;
    border-radius: 999px;
    display: inline-flex;
    align-items: center;
    gap: 4px;
  }
  .conn-pill.online {
    background: rgba(16, 185, 129, 0.15);
    color: #059669;
    border: 1px solid rgba(16, 185, 129, 0.35);
  }
  .conn-pill.offline {
    background: rgba(239, 68, 68, 0.15);
    color: #dc2626;
    border: 1px solid rgba(239, 68, 68, 0.35);
  }
  .status-tag {
    font-size: 0.68rem;
    font-weight: 700;
    background: rgba(59, 130, 246, 0.12);
    color: #2563eb;
    padding: 3px 8px;
    border-radius: 999px;
    border: 1px solid rgba(59, 130, 246, 0.25);
  }

  /* Schematic Container */
  .diagram-container {
    position: relative;
    width: 360px;
    height: 390px;
    margin: 0 auto;
  }
  .flow-svg {
    position: absolute;
    top: 0; left: 0;
    width: 100%; height: 100%;
    z-index: 1;
    pointer-events: none;
  }

  /* Dotted Guide Lines */
  .guide-path {
    fill: none;
    stroke-width: 2.5;
    stroke-linecap: round;
    stroke-linejoin: round;
  }

  /* Animated Moving Flow Pulses */
  .active-pulse {
    fill: none;
    stroke-width: 3.5;
    stroke-linecap: round;
    stroke-linejoin: round;
    stroke-dasharray: 6 14;
    animation: pulseFlow 1s linear infinite;
  }
  @keyframes pulseFlow {
    to { stroke-dashoffset: -20; }
  }

  /* Device Nodes */
  .node {
    position: absolute;
    width: 120px;
    display: flex;
    flex-direction: column;
    align-items: center;
    text-align: center;
    transform: translate(-50%, -50%);
    z-index: 2;
  }
  .node-icon-circle {
    width: 68px; height: 68px;
    border-radius: 50%;
    background: #ffffff;
    box-shadow: 0 8px 20px -4px rgba(0,0,0,0.1), 0 2px 6px -1px rgba(0,0,0,0.06);
    display: flex;
    align-items: center;
    justify-content: center;
    position: relative;
    margin-bottom: 6px;
    border: 2px solid;
    transition: transform 0.2s;
  }
  .node-icon-circle:hover { transform: scale(1.05); }

  /* Center Inverter Node */
  .node-inv {
    top: 50%; left: 50%;
  }
  .node-inv .node-icon-circle {
    width: 82px; height: 82px;
    border: 2.5px solid var(--inverter);
    outline: 5px solid rgba(249, 115, 22, 0.15);
  }

  /* Position & Colors */
  .node-solar { top: 18%; left: 22%; }
  .node-solar .node-icon-circle { border-color: var(--solar); }

  .node-grid { top: 18%; left: 78%; }
  .node-grid .node-icon-circle { border-color: var(--grid); }

  .node-bat { top: 76%; left: 22%; }
  .node-bat .node-icon-circle { border-color: var(--battery); }

  .node-load { top: 76%; left: 78%; }
  .node-load .node-icon-circle { border-color: var(--load); }

  /* Node Values */
  .val-hero {
    font-size: 1.25rem;
    font-weight: 800;
    color: var(--text-main);
    line-height: 1.1;
  }
  .val-sub {
    font-size: 0.72rem;
    color: var(--text-muted);
    font-weight: 500;
    margin-top: 2px;
  }
  .val-caption {
    font-size: 0.75rem;
    color: var(--text-muted);
    font-weight: 600;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 2px;
    margin-top: 1px;
  }
  .help-icon {
    display: inline-block;
    width: 13px; height: 13px;
    border-radius: 50%;
    border: 1px solid #94a3b8;
    color: #64748b;
    font-size: 9px;
    line-height: 12px;
  }

  /* Carousel Dots */
  .carousel-dots {
    display: flex;
    justify-content: center;
    align-items: center;
    gap: 6px;
    margin: 8px 0 10px 0;
  }
  .dot {
    width: 7px; height: 7px;
    border-radius: 50%;
    background: #cbd5e1;
  }
  .dot.active {
    width: 22px;
    border-radius: 999px;
    background: #f97316;
  }

  /* Bottom Wi-Fi Bar */
  .bottom-wifi {
    background: #ffffff;
    border-radius: 14px;
    padding: 10px 14px;
    box-shadow: 0 4px 12px rgba(0,0,0,0.06);
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 0.78rem;
  }
  .wifi-indicator {
    display: flex;
    align-items: center;
    gap: 6px;
    font-weight: 600;
    color: #334155;
  }
  .dot-green {
    width: 8px; height: 8px;
    border-radius: 50%;
    background: #10b981;
  }
  .dot-orange {
    width: 8px; height: 8px;
    border-radius: 50%;
    background: #f59e0b;
  }
  .btn-connect {
    background: #2563eb;
    color: white;
    border: none;
    padding: 6px 12px;
    border-radius: 8px;
    font-size: 0.75rem;
    font-weight: 700;
    cursor: pointer;
  }

  /* Settings & OTA Modal */
  .modal-overlay {
    display: none;
    position: fixed;
    top: 0; left: 0; right: 0; bottom: 0;
    background: rgba(0,0,0,0.6);
    z-index: 200;
    justify-content: center;
    align-items: center;
  }
  .modal-overlay.active { display: flex; }
  .modal {
    background: #ffffff;
    border-radius: 18px;
    padding: 20px;
    width: 92%;
    max-width: 400px;
    box-shadow: 0 20px 25px -5px rgba(0,0,0,0.3);
    max-height: 90vh;
    overflow-y: auto;
  }
  .modal h2 { font-size: 1.05rem; color: #0f172a; margin-bottom: 6px; }
  .modal select, .modal input {
    width: 100%;
    padding: 9px;
    margin: 6px 0;
    border: 1px solid #cbd5e1;
    border-radius: 8px;
    font-size: 0.85rem;
    box-sizing: border-box;
  }
  .modal-buttons { display: flex; justify-content: flex-end; gap: 8px; margin-top: 12px; }
  .btn-cancel { background: #94a3b8; color: white; border: none; padding: 7px 12px; border-radius: 8px; cursor: pointer; }
  .btn-save { background: #2563eb; color: white; border: none; padding: 7px 12px; border-radius: 8px; font-weight: bold; cursor: pointer; }
  .section-divider {
    border-top: 1px solid #e2e8f0;
    margin: 14px 0 10px 0;
    padding-top: 10px;
  }
  .section-title {
    font-size: 0.8rem;
    font-weight: 700;
    color: #475569;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    margin-bottom: 6px;
  }
</style>
</head>
<body>

<div class="app-frame">
  
  <!-- Live GitHub Update Available Banner -->
  <div id="otaBanner" class="ota-banner">
    <span>🚀 New Update (<span id="otaNewVer">v1.0.1</span>) Available!</span>
    <button class="btn-ota" onclick="doGitHubUpdate()">Update Now</button>
  </div>

  <div class="header-bar">
    <div>
      <h2>NEXT 6.2kW Hybrid</h2>
      <div style="margin-top:3px; display:flex; align-items:center; gap:5px;">
        <span id="invConnBadge" class="conn-pill offline">🔴 Inverter Disconnected</span>
        <span id="sysMode" class="status-tag">Standby</span>
      </div>
    </div>
    <div id="dataFeedIndicator" style="font-size:0.68rem; color:#64748b; text-align:right;">No Serial Feed</div>
  </div>

  <div class="diagram-container">
    <svg class="flow-svg" viewBox="0 0 360 390">
      <!-- 1. SOLAR LINE (Left Trunk x = 165) -->
      <path class="guide-path" d="M 114 70 H 151 Q 165 70 165 84 V 155" stroke="#fcd34d" stroke-dasharray="3 3" />
      <path id="pulseSolar" class="active-pulse" d="M 114 70 H 151 Q 165 70 165 84 V 155" stroke="#f59e0b" />

      <!-- 2. GRID LINE (Right Trunk x = 195) -->
      <path class="guide-path" d="M 246 70 H 209 Q 195 70 195 84 V 155" stroke="#cbd5e1" stroke-dasharray="3 3" />
      <path id="pulseGrid" class="active-pulse" d="M 246 70 H 209 Q 195 70 195 84 V 155" stroke="#2563eb" style="display:none;" />

      <!-- 3. BATTERY LINE (Left Trunk x = 165) -->
      <path class="guide-path" d="M 165 235 V 284 Q 165 298 151 298 H 114" stroke="#a7f3d0" stroke-dasharray="3 3" />
      <path id="pulseBattery" class="active-pulse" d="M 165 235 V 284 Q 165 298 151 298 H 114" stroke="#10b981" />

      <!-- 4. LOAD LINE (Right Trunk x = 195) -->
      <path class="guide-path" d="M 195 235 V 284 Q 195 298 209 298 H 246" stroke="#bfdbfe" stroke-dasharray="3 3" />
      <path id="pulseLoad" class="active-pulse" d="M 195 235 V 284 Q 195 298 209 298 H 246" stroke="#3b82f6" />
    </svg>

    <!-- Node 1: Solar (Top Left) -->
    <div class="node node-solar">
      <div class="node-icon-circle">
        <svg width="34" height="34" viewBox="0 0 24 24" fill="none" stroke="#f59e0b" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/>
        </svg>
      </div>
      <div class="val-hero" id="pvWatts">0<span style="font-size:0.9rem;">w</span></div>
      <div class="val-sub" id="pvSub">0.0V • 0.0A</div>
      <div class="val-caption">Solar <span class="help-icon">?</span></div>
    </div>

    <!-- Node 2: Grid (Top Right) -->
    <div class="node node-grid">
      <div class="node-icon-circle">
        <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="#2563eb" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <path d="M6 18h12"/><path d="M7 14h10"/><path d="M9 10h6"/><path d="m12 2 4 20"/><path d="m12 2-4 20"/><path d="M8 6h8"/><path d="m16 10-8 8"/><path d="m8 10 8 8"/>
        </svg>
      </div>
      <div class="val-hero" id="gridWatts">0<span style="font-size:0.9rem;">w</span></div>
      <div class="val-sub" id="gridSub">0.0V • 50Hz</div>
      <div class="val-caption">Grid <span class="help-icon">?</span></div>
    </div>

    <!-- Node 3: Center Inverter -->
    <div class="node node-inv">
      <div class="node-icon-circle">
        <svg width="42" height="42" viewBox="0 0 24 24" fill="none" stroke="#f97316" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <rect width="18" height="18" x="3" y="3" rx="4"/>
          <line x1="8" x2="8" y1="9" y2="9.01"/>
          <line x1="16" x2="16" y1="15" y2="15.01"/>
          <path d="M8 15c2-2 6-2 8 0"/>
          <path d="M8 9c2 2 6 2 8 0"/>
        </svg>
      </div>
      <div style="font-size:0.75rem; font-weight:700; color:#c2410c; margin-top: -4px;">6.2 kW</div>
      <div id="invTemp" style="font-size:0.65rem; color:var(--text-muted);">0.0 °C</div>
    </div>

    <!-- Node 4: Battery (Bottom Left) -->
    <div class="node node-bat">
      <div class="node-icon-circle">
        <svg width="34" height="34" viewBox="0 0 24 24" fill="none" stroke="#10b981" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <rect width="16" height="10" x="2" y="7" rx="2" ry="2"/>
          <line x1="22" x2="22" y1="11" y2="13"/>
          <line x1="6" x2="6" y1="11" y2="13"/>
          <line x1="10" x2="10" y1="11" y2="13"/>
          <line x1="14" x2="14" y1="11" y2="13"/>
        </svg>
      </div>
      <div class="val-hero" id="batHero">0<span style="font-size:0.85rem;">w</span> • 0<span style="font-size:0.85rem;">%</span></div>
      <div class="val-sub" id="batSub">0.0V • 0.0A</div>
      <div class="val-caption" id="batCaption">Battery <span class="help-icon">?</span></div>
    </div>

    <!-- Node 5: Household Consuming (Bottom Right) -->
    <div class="node node-load">
      <div class="node-icon-circle">
        <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="#3b82f6" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <path d="m3 9 9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>
          <polyline points="13 10 11 14 13 14 11 18"/>
        </svg>
      </div>
      <div class="val-hero" id="loadWatts">0<span style="font-size:0.9rem;">w</span></div>
      <div class="val-sub" id="loadSub">230.0V • 0.0A</div>
      <div class="val-caption">Consuming <span class="help-icon">?</span></div>
    </div>

  </div>

  <!-- Carousel Pagination Dots -->
  <div class="carousel-dots">
    <div class="dot active"></div>
    <div class="dot"></div>
  </div>

  <!-- Bottom Wi-Fi & Settings Bar -->
  <div class="bottom-wifi">
    <div class="wifi-indicator">
      <span id="wifiDot" class="dot-green"></span>
      <span id="wifiLabel">Connecting to Wi-Fi...</span>
    </div>
    <button class="btn-connect" onclick="openWifiModal()">⚙️ Settings & Wi-Fi</button>
  </div>

</div>

<!-- Settings & Wireless OTA Modal -->
<div id="wifiModal" class="modal-overlay">
  <div class="modal">
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:8px;">
      <h2>Settings & Firmware</h2>
      <button class="btn-cancel" onclick="closeWifiModal()" style="font-size:0.75rem;">✕</button>
    </div>

    <!-- Wi-Fi Section -->
    <div class="section-title">Wi-Fi Network</div>
    <div style="display:flex; justify-content:space-between; align-items:center;">
      <span id="scanStatus" style="font-size:0.75rem; color:#f59e0b;">🔍 Scanning nearby networks...</span>
      <button class="btn-cancel" onclick="scanWifiNetworks()" style="font-size:0.7rem; padding:3px 6px;">🔄 Rescan</button>
    </div>
    <select id="wifiSSID" onchange="checkManualInput()">
      <option value="" disabled selected>-- Select a Wi-Fi Network --</option>
    </select>
    <div id="manualInputContainer" style="display:none;">
      <input type="text" id="wifiSSIDManual" placeholder="Enter Network Name (SSID)">
    </div>
    <input type="password" id="wifiPass" placeholder="Wi-Fi Password">
    <button class="btn-save" style="width:100%; margin-top:4px;" onclick="saveWifiCredentials()">Save & Connect Wi-Fi</button>

    <!-- GitHub Wireless OTA Section -->
    <div class="section-divider"></div>
    <div class="section-title">Wireless Firmware Update (OTA)</div>
    <p style="font-size:0.75rem; color:#64748b;">Current Firmware: <b id="curVer">v1.0.0</b></p>
    
    <div style="margin-top:6px;">
      <label style="font-size:0.75rem; color:#475569;">GitHub Repository (e.g. <i>username/next-inverter</i>):</label>
      <input type="text" id="githubRepoInput" placeholder="your-username/repository">
      <button class="btn-save" style="background:#059669; width:100%;" onclick="saveAndCheckGitHub()">🔍 Save & Check GitHub Updates</button>
    </div>

    <!-- Direct Browser File Flash -->
    <div style="margin-top:10px; padding:10px; background:#f1f5f9; border-radius:10px;">
      <label style="font-size:0.75rem; font-weight:700; color:#475569;">Or Flash Local .bin Directly:</label>
      <input type="file" id="localBinFile" accept=".bin" style="font-size:0.75rem; padding:4px;">
      <button class="btn-save" style="background:#475569; width:100%;" onclick="uploadBrowserBin()">⚡ Flash Selected .bin Wirelessly</button>
      <div id="otaProgressBarBox" style="display:none; width:100%; height:6px; background:#cbd5e1; border-radius:3px; margin-top:8px;">
        <div id="otaProgressBar" style="width:0%; height:100%; background:#2563eb; border-radius:3px;"></div>
      </div>
    </div>

    <div id="modalMsg" style="font-size:0.78rem; margin-top:8px; color:#f59e0b;"></div>
  </div>
</div>

<script>
async function refreshData() {
  try {
    const res = await fetch('/api/telemetry');
    if (!res.ok) throw new Error("HTTP Error");
    const d = await res.json();
    
    // Inverter Connection Status
    const connBadge = document.getElementById('invConnBadge');
    const feedIndicator = document.getElementById('dataFeedIndicator');
    if (d.valid) {
      connBadge.className = 'conn-pill online';
      connBadge.textContent = '🟢 Inverter Connected';
      feedIndicator.textContent = 'Live Feed: ' + new Date().toLocaleTimeString();
      feedIndicator.style.color = '#059669';
    } else {
      connBadge.className = 'conn-pill offline';
      connBadge.textContent = '🔴 Inverter Disconnected';
      feedIndicator.textContent = 'No Serial Feed';
      feedIndicator.style.color = '#dc2626';
    }

    // Status Tag
    document.getElementById('sysMode').textContent = d.mode;

    // Solar
    const pvW = Math.round(d.pv_watts);
    document.getElementById('pvWatts').innerHTML = pvW + '<span style="font-size:0.9rem;">w</span>';
    document.getElementById('pvSub').textContent = d.pv_voltage.toFixed(1) + "V • " + d.pv_current.toFixed(2) + "A";
    document.getElementById('pulseSolar').style.display = (pvW > 10) ? 'block' : 'none';

    // Grid
    document.getElementById('gridWatts').innerHTML = '0<span style="font-size:0.9rem;">w</span>';
    document.getElementById('gridSub').textContent = d.grid_voltage.toFixed(1) + "V • " + d.grid_frequency.toFixed(1) + "Hz";

    // Inverter
    document.getElementById('invTemp').textContent = d.heatsink_temp.toFixed(1) + " °C";

    // Battery
    const batW = Math.abs(Math.round(d.battery_power_watts));
    document.getElementById('batHero').innerHTML = batW + '<span style="font-size:0.85rem;">w</span> • ' + d.battery_soc + '<span style="font-size:0.85rem;">%</span>';
    const batA = (d.battery_charge_current > 0 ? d.battery_charge_current : d.battery_discharge_current);
    document.getElementById('batSub').textContent = d.battery_voltage.toFixed(1) + "V • " + batA + "A";
    
    if (d.battery_charge_current > 0) {
      document.getElementById('batCaption').innerHTML = 'Charge <span class="help-icon">?</span>';
      document.getElementById('pulseBattery').style.display = 'block';
    } else if (d.battery_discharge_current > 0) {
      document.getElementById('batCaption').innerHTML = 'Discharge <span class="help-icon">?</span>';
      document.getElementById('pulseBattery').style.display = 'block';
    } else {
      document.getElementById('batCaption').innerHTML = 'Standby <span class="help-icon">?</span>';
      document.getElementById('pulseBattery').style.display = 'none';
    }

    // Load
    const loadW = d.ac_output_watts;
    document.getElementById('loadWatts').innerHTML = loadW + '<span style="font-size:0.9rem;">w</span>';
    const loadA = (d.ac_output_voltage > 0 ? (loadW / d.ac_output_voltage).toFixed(2) : "0.00");
    document.getElementById('loadSub').textContent = d.ac_output_voltage.toFixed(1) + "V • " + loadA + "A";
    document.getElementById('pulseLoad').style.display = (loadW > 10) ? 'block' : 'none';

    // Wi-Fi Status
    const dot = document.getElementById('wifiDot');
    const lbl = document.getElementById('wifiLabel');
    if (d.wifi_connected) {
      dot.className = 'dot-green';
      lbl.textContent = 'Wi-Fi: "' + d.wifi_ssid + '" (' + d.wifi_ip + ')';
    } else if (d.ap_mode) {
      dot.className = 'dot-orange';
      lbl.textContent = 'AP Mode: "' + d.ap_ssid + '" (192.168.4.1)';
    } else {
      dot.className = 'dot-orange';
      lbl.textContent = 'Wi-Fi: Disconnected';
    }

    // OTA Update Banner
    if (d.update_available) {
      document.getElementById('otaBanner').style.display = 'flex';
      document.getElementById('otaNewVer').textContent = 'v' + d.latest_version;
    } else {
      document.getElementById('otaBanner').style.display = 'none';
    }
    document.getElementById('curVer').textContent = 'v' + d.firmware_version;
    if (d.github_repo && !document.getElementById('githubRepoInput').value) {
      document.getElementById('githubRepoInput').value = d.github_repo;
    }

  } catch(e) {
    const connBadge = document.getElementById('invConnBadge');
    if (connBadge) {
      connBadge.className = 'conn-pill offline';
      connBadge.textContent = '🔴 ESP32 Offline';
    }
    const feedIndicator = document.getElementById('dataFeedIndicator');
    if (feedIndicator) {
      feedIndicator.textContent = 'Check Wi-Fi';
      feedIndicator.style.color = '#dc2626';
    }
    document.getElementById('sysMode').textContent = 'Connecting...';
  }
}

function openWifiModal() {
  document.getElementById('wifiModal').classList.add('active');
  scanWifiNetworks();
}

function closeWifiModal() {
  document.getElementById('wifiModal').classList.remove('active');
  document.getElementById('modalMsg').textContent = '';
}

function checkManualInput() {
  const select = document.getElementById('wifiSSID');
  const manualBox = document.getElementById('manualInputContainer');
  manualBox.style.display = (select.value === '__manual__') ? 'block' : 'none';
}

async function scanWifiNetworks() {
  const statusEl = document.getElementById('scanStatus');
  const select = document.getElementById('wifiSSID');
  statusEl.textContent = "🔍 Scanning nearby networks...";
  select.innerHTML = '<option value="" disabled selected>Scanning...</option>';

  try {
    const res = await fetch('/api/wifi_scan');
    const networks = await res.json();
    
    select.innerHTML = '<option value="" disabled selected>-- Select a Wi-Fi Network --</option>';
    if (networks.length === 0) {
      statusEl.textContent = "No networks found. Try rescanning.";
    } else {
      statusEl.textContent = "Found " + networks.length + " networks:";
      networks.forEach(net => {
        const lock = net.secure ? ' 🔒' : ' 🔓';
        const opt = document.createElement('option');
        opt.value = net.ssid;
        opt.textContent = `${net.ssid} (${net.rssi} dBm)${lock}`;
        select.appendChild(opt);
      });
    }
    const manualOpt = document.createElement('option');
    manualOpt.value = '__manual__';
    manualOpt.textContent = '✏️ Enter hidden SSID manually...';
    select.appendChild(manualOpt);

  } catch(e) {
    statusEl.textContent = "Scan error. Enter network name manually.";
    select.innerHTML = '<option value="__manual__">✏️ Enter SSID manually</option>';
    checkManualInput();
  }
}

async function saveWifiCredentials() {
  const select = document.getElementById('wifiSSID');
  let ssid = select.value;
  if (ssid === '__manual__') {
    ssid = document.getElementById('wifiSSIDManual').value.trim();
  }
  const pass = document.getElementById('wifiPass').value;
  const msgEl = document.getElementById('modalMsg');

  if (!ssid) {
    msgEl.textContent = "Please select a Wi-Fi network";
    return;
  }

  msgEl.textContent = "Saving & connecting to " + ssid + "...";
  const formData = new FormData();
  formData.append('ssid', ssid);
  formData.append('password', pass);

  try {
    const res = await fetch('/wifi_save', { method: 'POST', body: formData });
    const text = await res.text();
    msgEl.textContent = text;
    setTimeout(() => { closeWifiModal(); location.reload(); }, 6000);
  } catch(e) {
    msgEl.textContent = "ESP32 connecting. Returning to AP mode if failed.";
    setTimeout(() => { closeWifiModal(); }, 6000);
  }
}

async function saveAndCheckGitHub() {
  const repo = document.getElementById('githubRepoInput').value.trim();
  const msgEl = document.getElementById('modalMsg');
  if (!repo) {
    msgEl.textContent = "Please enter GitHub username/repository";
    return;
  }
  msgEl.textContent = "Checking GitHub repository for updates...";
  const fd = new FormData();
  fd.append('repo', repo);
  try {
    await fetch('/api/save_github_repo', { method: 'POST', body: fd });
    const res = await fetch('/api/check_update');
    const d = await res.json();
    if (d.update_available) {
      msgEl.textContent = `🚀 Update v${d.latest_version} found! Notes: ${d.notes || 'Ready to install'}`;
    } else {
      msgEl.textContent = `✅ Up to date (v${d.current_version}).`;
    }
  } catch(e) {
    msgEl.textContent = "Error checking GitHub. Verify repository name.";
  }
}

async function doGitHubUpdate() {
  if (!confirm("Start wireless firmware update from GitHub? The ESP32 will reboot in ~20 seconds.")) return;
  alert("Updating firmware wirelessly... The page will reload in 25 seconds.");
  try {
    await fetch('/api/do_github_update', { method: 'POST' });
  } catch(e) {}
  setTimeout(() => { location.reload(); }, 25000);
}

function uploadBrowserBin() {
  const fileInput = document.getElementById('localBinFile');
  const msgEl = document.getElementById('modalMsg');
  const barBox = document.getElementById('otaProgressBarBox');
  const bar = document.getElementById('otaProgressBar');

  if (!fileInput.files || fileInput.files.length === 0) {
    msgEl.textContent = "Please select a .bin firmware file first";
    return;
  }

  const file = fileInput.files[0];
  if (!file.name.endsWith('.bin')) {
    msgEl.textContent = "Selected file must be a .bin file";
    return;
  }

  msgEl.textContent = "Uploading " + file.name + " wirelessly...";
  barBox.style.display = 'block';
  bar.style.width = '0%';

  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/update', true);

  xhr.upload.onprogress = function(e) {
    if (e.lengthComputable) {
      const pct = Math.round((e.loaded / e.total) * 100);
      bar.style.width = pct + '%';
      msgEl.textContent = `Flashing: ${pct}%... Do not unplug power!`;
    }
  };

  xhr.onload = function() {
    if (xhr.status === 200 && xhr.responseText === 'OK') {
      msgEl.textContent = "✅ Success! ESP32 is rebooting into the new firmware. Reloading...";
      setTimeout(() => { location.reload(); }, 8000);
    } else {
      msgEl.textContent = "❌ Flash failed: " + xhr.responseText;
    }
  };

  xhr.onerror = function() {
    msgEl.textContent = "Upload error. Check connection.";
  };

  const fd = new FormData();
  fd.append('update', file);
  xhr.send(fd);
}

setInterval(refreshData, 2000);
refreshData();
</script>
</body>
</html>
)rawliteral";

const char WIFI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Wi-Fi Configuration</title>
<style>
  body { background:#0f172a; color:#f8fafc; font-family:sans-serif; padding:20px; max-width:400px; margin:0 auto; }
  select, input, button { width:100%; padding:12px; margin:8px 0; border-radius:6px; border:1px solid #334155; background:#1e293b; color:#fff; box-sizing:border-box; font-size:1rem; }
  button { background:#2563eb; font-weight:bold; cursor:pointer; }
  button:hover { background:#1d4ed8; }
  a { color:#3b82f6; text-decoration:none; }
</style>
</head>
<body>
<h2>Wi-Fi Setup</h2>
<p id="scanLabel" style="color:#94a3b8; font-size:0.85rem;">🔍 Scanning available networks...</p>
<form action="/wifi_save" method="POST">
  <select id="wifiSelect" name="ssid" required>
    <option value="" disabled selected>Scanning...</option>
  </select>
  <input type="password" name="password" placeholder="Wi-Fi Password">
  <button type="submit">Save & Connect</button>
</form>
<p style="margin-top:20px; font-size:0.9rem;"><a href="/">← Back to Solar Dashboard</a></p>
<script>
fetch('/api/wifi_scan').then(r => r.json()).then(nets => {
  const s = document.getElementById('wifiSelect');
  s.innerHTML = '<option value="" disabled selected>-- Select Your Wi-Fi --</option>';
  nets.forEach(n => {
    const o = document.createElement('option');
    o.value = n.ssid;
    o.textContent = n.ssid + ' (' + n.rssi + ' dBm)' + (n.secure ? ' 🔒' : '');
    s.appendChild(o);
  });
  document.getElementById('scanLabel').textContent = 'Select your network and enter password:';
}).catch(() => {
  document.getElementById('scanLabel').textContent = 'Enter network details:';
});
</script>
</body>
</html>
)rawliteral";
