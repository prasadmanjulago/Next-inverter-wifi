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
    background: #0b1324;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    justify-content: flex-start;
    align-items: center;
    padding: 16px 12px;
  }

  /* Top Control & Card Alignment Bar */
  .top-control-bar {
    width: 100%;
    max-width: 460px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 8px;
    margin-bottom: 14px;
    padding: 7px 12px;
    background: rgba(15, 23, 42, 0.75);
    backdrop-filter: blur(8px);
    border: 1px solid rgba(255, 255, 255, 0.12);
    border-radius: 14px;
    box-sizing: border-box;
    transition: max-width 0.25s ease;
  }
  .top-brand {
    display: flex;
    align-items: center;
    gap: 6px;
    color: #f8fafc;
    font-size: 0.82rem;
    font-weight: 800;
    letter-spacing: 0.3px;
  }
  .layout-switch-bar {
    display: flex;
    align-items: center;
    gap: 6px;
  }
  .layout-switch-label {
    font-size: 0.68rem;
    font-weight: 700;
    color: #94a3b8;
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }
  .layout-switch-buttons {
    display: inline-flex;
    background: rgba(30, 41, 59, 0.85);
    border: 1px solid rgba(255, 255, 255, 0.15);
    border-radius: 8px;
    padding: 2px;
    gap: 2px;
  }
  .btn-layout-opt {
    background: transparent;
    color: #94a3b8;
    border: none;
    padding: 4px 8px;
    border-radius: 6px;
    font-size: 0.70rem;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.15s ease;
  }
  .btn-layout-opt:hover {
    color: #ffffff;
    background: rgba(255, 255, 255, 0.1);
  }
  .btn-layout-opt.active {
    background: #2563eb;
    color: #ffffff;
    box-shadow: 0 1px 4px rgba(37,99,235,0.4);
  }

  .page-wrapper {
    width: 100%;
    max-width: 460px;
    display: flex;
    flex-direction: column;
    gap: 16px;
    margin: 0 auto;
    transition: max-width 0.25s ease;
  }
  .dashboard-col {
    display: flex;
    flex-direction: column;
    gap: 16px;
    width: 100%;
    min-width: 0;
  }

  /* Tablet & Medium Screen (860px - 1239px): 2 Columns */
  @media (min-width: 860px) {
    body {
      padding: 24px 20px;
    }
    .top-control-bar {
      max-width: 1100px;
    }
    .page-wrapper {
      max-width: 1100px;
      display: grid;
      grid-template-columns: 420px 1fr;
      align-items: start;
      gap: 20px;
    }
    .app-frame {
      grid-column: 1;
    }
    .dashboard-col {
      grid-column: 2;
    }
    .weather-card {
      grid-column: 1 / -1;
    }
  }

  /* Large Desktop & Widescreen (>= 1240px): 3 Columns */
  @media (min-width: 1240px) {
    .top-control-bar {
      max-width: 1600px;
    }
    .page-wrapper {
      max-width: 1600px;
      display: grid;
      grid-template-columns: 420px minmax(400px, 460px) minmax(440px, 1fr);
      align-items: start;
      gap: 22px;
    }
    .app-frame {
      grid-column: 1;
    }
    .dashboard-col {
      grid-column: 2;
    }
    .weather-card {
      grid-column: 3;
    }
  }

  /* User-Selected Layout Overrides */
  .layout-1col .top-control-bar,
  .page-wrapper.layout-1col {
    max-width: 480px !important;
  }
  .page-wrapper.layout-1col {
    display: flex !important;
    flex-direction: column !important;
    gap: 16px !important;
    margin: 0 auto !important;
  }
  .page-wrapper.layout-1col .app-frame,
  .page-wrapper.layout-1col .dashboard-col,
  .page-wrapper.layout-1col .weather-card {
    grid-column: auto !important;
    width: 100% !important;
  }

  .layout-2col .top-control-bar,
  .page-wrapper.layout-2col {
    max-width: 1100px !important;
  }
  .page-wrapper.layout-2col {
    display: grid !important;
    grid-template-columns: 420px 1fr !important;
    align-items: start !important;
    gap: 20px !important;
    margin: 0 auto !important;
  }
  .page-wrapper.layout-2col .app-frame { grid-column: 1 !important; }
  .page-wrapper.layout-2col .dashboard-col { grid-column: 2 !important; }
  .page-wrapper.layout-2col .weather-card { grid-column: 1 / -1 !important; }

  .layout-3col .top-control-bar,
  .page-wrapper.layout-3col {
    max-width: 1600px !important;
  }
  .page-wrapper.layout-3col {
    display: grid !important;
    grid-template-columns: 420px minmax(400px, 460px) minmax(440px, 1fr) !important;
    align-items: start !important;
    gap: 22px !important;
    margin: 0 auto !important;
  }
  .page-wrapper.layout-3col .app-frame { grid-column: 1 !important; }
  .page-wrapper.layout-3col .dashboard-col { grid-column: 2 !important; }
  .page-wrapper.layout-3col .weather-card { grid-column: 3 !important; }
  .app-frame {
    width: 100%;
    background: linear-gradient(180deg, var(--bg-top) 0%, var(--bg-mid) 45%, var(--bg-bot) 100%);
    border-radius: 28px;
    box-shadow: 0 25px 50px -12px rgba(0,0,0,0.5);
    overflow: hidden;
    position: relative;
    padding: 18px 16px 16px 16px;
    box-sizing: border-box;
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
  .active-pulse.reverse {
    animation-direction: reverse;
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

  /* Serial Diagnostics Panel */
  .diag-card {
    display: none;
    background: #0f172a;
    border: 1px solid rgba(255,255,255,0.1);
    border-radius: 14px;
    padding: 10px 12px;
    margin-bottom: 10px;
    color: #f8fafc;
    font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace;
    font-size: 0.72rem;
    box-shadow: 0 4px 12px rgba(0,0,0,0.15);
  }
  .diag-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 4px;
  }
  .diag-box {
    background: #020617;
    border: 1px solid #1e293b;
    border-radius: 6px;
    padding: 5px 7px;
    word-break: break-all;
    font-size: 0.68rem;
    min-height: 18px;
    margin-top: 2px;
  }
  .diag-btn-group {
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
    margin-top: 8px;
  }
  .btn-diag {
    background: #1e293b;
    color: #f1f5f9;
    border: 1px solid #334155;
    border-radius: 6px;
    padding: 5px 8px;
    font-size: 0.68rem;
    font-weight: 600;
    cursor: pointer;
    transition: background 0.15s;
    font-family: inherit;
  }
  .btn-diag:hover {
    background: #334155;
  }
  .btn-diag:active {
    transform: scale(0.97);
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

  /* Standalone Inverter Mode Macros Card (Card 2) */
  .macro-card {
    background: #ffffff;
    border-radius: 26px;
    padding: 18px 16px 16px 16px;
    box-shadow: 0 20px 45px -10px rgba(0,0,0,0.45);
    border: 1px solid rgba(255, 255, 255, 0.2);
    width: 100%;
    box-sizing: border-box;
  }
  .macro-card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 10px;
  }
  .macro-title {
    font-size: 0.92rem;
    font-weight: 700;
    color: #0f172a;
    display: flex;
    align-items: center;
    gap: 5px;
  }
  .macro-subtitle {
    font-size: 0.68rem;
    color: #64748b;
    margin-top: 1px;
    font-weight: 500;
  }
  .btn-create-macro {
    background: linear-gradient(135deg, #2563eb, #1d4ed8);
    color: #ffffff;
    border: none;
    padding: 6px 12px;
    border-radius: 8px;
    font-size: 0.74rem;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.2s;
    box-shadow: 0 2px 6px rgba(37,99,235,0.25);
  }
  .btn-create-macro:hover {
    background: linear-gradient(135deg, #1d4ed8, #1e40af);
    transform: translateY(-1px);
  }
  .macro-status-banner {
    display: none;
    padding: 8px 12px;
    border-radius: 10px;
    font-size: 0.74rem;
    font-weight: 600;
    margin-bottom: 10px;
    line-height: 1.35;
  }
  .macro-status-banner.running {
    display: block;
    background: #eff6ff;
    color: #1e40af;
    border: 1px solid #bfdbfe;
  }
  .macro-status-banner.success {
    display: block;
    background: #ecfdf5;
    color: #065f46;
    border: 1px solid #a7f3d0;
  }
  .macro-status-banner.error {
    display: block;
    background: #fef2f2;
    color: #991b1b;
    border: 1px solid #fecaca;
  }
  .macros-list {
    display: flex;
    flex-direction: column;
    gap: 8px;
  }
  .macro-item {
    background: #f8fafc;
    border: 1px solid #e2e8f0;
    border-radius: 12px;
    padding: 10px 12px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 10px;
    transition: all 0.2s;
  }
  .macro-item:hover {
    border-color: #cbd5e1;
    background: #f1f5f9;
  }
  .macro-info {
    flex: 1;
    min-width: 0;
  }
  .macro-name {
    font-size: 0.82rem;
    font-weight: 700;
    color: #1e293b;
    margin-bottom: 3px;
    display: flex;
    align-items: center;
    gap: 6px;
  }
  .macro-desc {
    font-size: 0.68rem;
    color: #64748b;
    line-height: 1.35;
    word-break: break-word;
  }
  .macro-actions {
    display: flex;
    align-items: center;
    gap: 6px;
    flex-shrink: 0;
  }
  .btn-apply-macro {
    background: #059669;
    color: #ffffff;
    border: none;
    padding: 6px 12px;
    border-radius: 8px;
    font-size: 0.74rem;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.15s;
    white-space: nowrap;
    box-shadow: 0 2px 4px rgba(5,150,105,0.2);
  }
  .btn-apply-macro:hover {
    background: #047857;
  }
  .btn-apply-macro:disabled {
    background: #94a3b8;
    cursor: not-allowed;
  }
  .btn-icon-macro {
    background: #ffffff;
    border: 1px solid #cbd5e1;
    border-radius: 6px;
    color: #64748b;
    padding: 4px 6px;
    font-size: 0.72rem;
    cursor: pointer;
    line-height: 1;
    transition: all 0.15s;
  }
  .btn-icon-macro:hover {
    background: #e2e8f0;
    color: #0f172a;
  }

  /* JK-BMS 14S Battery Monitor Card */
  .bms-card {
    background: #ffffff;
    border-radius: 20px;
    box-shadow: 0 10px 25px -5px rgba(0,0,0,0.06), 0 8px 10px -6px rgba(0,0,0,0.03);
    padding: 16px 12px;
    width: 100%;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    gap: 12px;
    overflow: hidden;
  }
  .bms-card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    border-bottom: 1px solid #f1f5f9;
    padding-bottom: 8px;
    flex-wrap: wrap;
    gap: 8px;
  }
  .bms-title {
    font-size: 0.95rem;
    font-weight: 800;
    color: #0f172a;
    display: flex;
    align-items: center;
    gap: 6px;
  }
  .bms-subtitle {
    font-size: 0.7rem;
    color: #64748b;
    margin-top: 2px;
  }
  .bms-header-actions {
    display: inline-flex;
    align-items: center;
    gap: 5px;
    flex-wrap: wrap;
  }
  .btn-bms-action {
    border: none;
    padding: 5px 9px;
    border-radius: 8px;
    font-size: 0.7rem;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.15s ease-in-out;
    display: inline-flex;
    align-items: center;
    gap: 4px;
    box-shadow: 0 1px 2px rgba(0,0,0,0.05);
  }
  .btn-bms-on {
    background: #f0fdf4;
    color: #166534;
    border: 1px solid #bbf7d0;
  }
  .btn-bms-on:hover {
    background: #16a34a;
    color: #ffffff;
    border-color: #16a34a;
  }
  .btn-bms-on.active {
    background: #16a34a;
    color: #ffffff;
    border-color: #15803d;
    box-shadow: inset 0 2px 4px rgba(0,0,0,0.15);
  }
  .btn-bms-off {
    background: #f8fafc;
    color: #64748b;
    border: 1px solid #cbd5e1;
  }
  .btn-bms-off:hover {
    background: #ef4444;
    color: #ffffff;
    border-color: #ef4444;
  }
  .btn-bms-off.active {
    background: #64748b;
    color: #ffffff;
    border-color: #475569;
    box-shadow: inset 0 2px 4px rgba(0,0,0,0.15);
  }
  .btn-setup-bms {
    background: #0f172a;
    color: #ffffff;
    border: none;
    padding: 5px 9px;
    border-radius: 8px;
    font-size: 0.7rem;
    font-weight: 700;
    cursor: pointer;
    transition: all 0.15s;
    display: inline-flex;
    align-items: center;
    gap: 4px;
  }
  .btn-setup-bms:hover {
    background: #1e293b;
  }
  .bms-metrics-row {
    display: grid;
    grid-template-columns: repeat(5, minmax(0, 1fr));
    gap: 5px;
  }
  .bms-volt-box {
    background: #f0fdf4 !important;
    border: 1.5px solid #86efac !important;
  }
  .bms-volt-box .bms-metric-val {
    color: #15803d !important;
    font-weight: 800;
  }
  .bms-volt-badge {
    background: #dcfce7;
    color: #166534;
    border: 1px solid #86efac;
    padding: 2px 7px;
    border-radius: 6px;
    font-size: 0.74rem;
    font-weight: 800;
    margin-left: 6px;
    letter-spacing: -0.2px;
    display: inline-block;
  }
  .bms-metric-box {
    background: #f8fafc;
    border: 1px solid #e2e8f0;
    border-radius: 10px;
    padding: 7px 4px;
    text-align: center;
    transition: all 0.2s;
  }
  .bms-metric-label {
    font-size: 0.58rem;
    font-weight: 700;
    color: #64748b;
    text-transform: uppercase;
    letter-spacing: 0.3px;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }
  .bms-metric-val {
    font-size: 0.96rem;
    font-weight: 800;
    color: #0f172a;
    margin: 2px 0;
  }
  .bms-metric-sub {
    font-size: 0.6rem;
    color: #94a3b8;
  }
  .bms-delta-box.balanced {
    background: #ecfdf5 !important;
    border-color: #10b981 !important;
  }
  .bms-delta-box.balanced .bms-metric-val {
    color: #059669 !important;
  }
  .bms-delta-box.moderate {
    background: #fffbeb !important;
    border-color: #f59e0b !important;
  }
  .bms-delta-box.moderate .bms-metric-val {
    color: #d97706 !important;
  }
  .bms-delta-box.imbalanced {
    background: #fef2f2 !important;
    border-color: #ef4444 !important;
  }
  .bms-delta-box.imbalanced .bms-metric-val {
    color: #dc2626 !important;
  }
  .bms-cells-title {
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 0.76rem;
    font-weight: 700;
    color: #334155;
    margin-top: 2px;
  }
  .bms-cells-grid {
    display: grid;
    grid-template-columns: repeat(7, minmax(0, 1fr));
    gap: 4px;
    width: 100%;
    box-sizing: border-box;
  }
  @media (max-width: 480px) {
    .bms-cells-grid {
      gap: 3px;
    }
    .bms-cell-box {
      padding: 4px 1px !important;
    }
    .bms-cell-volt {
      font-size: 0.68rem !important;
    }
    .bms-metrics-row {
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 5px;
    }
    .bms-volt-box {
      grid-column: 1 / -1;
    }
  }
  .bms-cell-box {
    background: #f8fafc;
    border: 1px solid #e2e8f0;
    border-radius: 6px;
    padding: 5px 2px;
    text-align: center;
    position: relative;
    transition: all 0.2s;
    min-width: 0;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
  }
  .bms-cell-box.highest {
    background: #eff6ff !important;
    border-color: #2563eb !important;
    box-shadow: 0 0 0 1.5px rgba(37,99,235,0.25);
  }
  .bms-cell-box.highest .bms-cell-volt {
    color: #1d4ed8 !important;
    font-weight: 800;
  }
  .bms-cell-box.lowest {
    background: #fef2f2 !important;
    border-color: #dc2626 !important;
    box-shadow: 0 0 0 1.5px rgba(220,38,38,0.25);
  }
  .bms-cell-box.lowest .bms-cell-volt {
    color: #b91c1c !important;
    font-weight: 800;
  }
  .bms-cell-badge {
    position: absolute;
    top: -5px;
    right: 1px;
    font-size: 0.44rem;
    font-weight: 800;
    padding: 1px 2px;
    border-radius: 3px;
    text-transform: uppercase;
    line-height: 1;
    pointer-events: none;
  }
  .bms-cell-badge.max {
    background: #2563eb;
    color: #ffffff;
  }
  .bms-cell-badge.min {
    background: #dc2626;
    color: #ffffff;
  }
  .bms-cell-label {
    font-size: 0.58rem;
    font-weight: 700;
    color: #64748b;
    margin-bottom: 1px;
    line-height: 1;
  }
  .bms-cell-volt {
    font-size: 0.72rem;
    font-weight: 700;
    color: #0f172a;
    letter-spacing: -0.4px;
    white-space: nowrap;
    line-height: 1.1;
  }
  .bms-cell-bar-bg {
    width: 82%;
    height: 3px;
    background: #e2e8f0;
    border-radius: 2px;
    margin-top: 3px;
    overflow: hidden;
  }
  .bms-cell-bar-fill {
    height: 100%;
    background: #059669;
    border-radius: 2px;
    transition: width 0.3s ease;
  }
  .bms-cell-box.highest .bms-cell-bar-fill {
    background: #2563eb;
  }
  .bms-cell-box.lowest .bms-cell-bar-fill {
    background: #dc2626;
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
  }
  /* Card 4: 24h Solar & Weather Forecast Card (Broadcast TV Graphic Theme) */
  .weather-card {
    background: #ffffff;
    border-radius: 26px;
    padding: 16px 14px 14px 14px;
    box-shadow: 0 20px 45px -10px rgba(0,0,0,0.45);
    border: 1px solid rgba(255, 255, 255, 0.2);
    width: 100%;
    box-sizing: border-box;
  }
  
  /* TV Broadcast Header Bar */
  .weather-broadcast-header {
    background: repeating-linear-gradient(-45deg, rgba(255,255,255,0.04), rgba(255,255,255,0.04) 8px, transparent 8px, transparent 16px),
                linear-gradient(90deg, #162f55 0%, #1c3c6d 50%, #1e4b8a 100%);
    border-radius: 16px;
    padding: 10px 14px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 12px;
    box-shadow: 0 4px 12px rgba(22,47,85,0.25);
    color: #ffffff;
  }
  .broadcast-station-info {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }
  .broadcast-channel-title {
    font-size: 0.88rem;
    font-weight: 800;
    letter-spacing: 0.6px;
    display: flex;
    align-items: center;
    gap: 6px;
    text-transform: uppercase;
  }
  .broadcast-live-badge {
    background: #ef4444;
    color: #ffffff;
    font-size: 0.58rem;
    font-weight: 900;
    padding: 2px 5px;
    border-radius: 4px;
    letter-spacing: 0.5px;
    animation: livePulse 2s infinite;
  }
  @keyframes livePulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.6; }
  }
  .broadcast-timeline-sub {
    font-size: 0.64rem;
    color: #93c5fd;
    font-weight: 600;
    letter-spacing: 0.3px;
  }

  /* Two-tone TV City Badge (matching BROOKSVILLE | WEATHER ON THE 9s) */
  .broadcast-city-badge {
    display: inline-flex;
    align-items: stretch;
    border: 1px solid rgba(255,255,255,0.4);
    border-radius: 8px;
    overflow: hidden;
    cursor: pointer;
    box-shadow: 0 2px 6px rgba(0,0,0,0.2);
    transition: transform 0.15s, box-shadow 0.15s;
    user-select: none;
  }
  .broadcast-city-badge:hover {
    transform: translateY(-1px);
    box-shadow: 0 4px 10px rgba(0,0,0,0.3);
  }
  .city-name-box {
    background: #ffffff;
    color: #0f172a;
    font-size: 0.76rem;
    font-weight: 800;
    padding: 5px 9px;
    letter-spacing: 0.5px;
    text-transform: uppercase;
    display: flex;
    align-items: center;
  }
  .city-tag-box {
    background: #0f274a;
    color: #ffffff;
    font-size: 0.68rem;
    font-weight: 800;
    padding: 5px 8px;
    letter-spacing: 0.5px;
    display: flex;
    align-items: center;
    gap: 2px;
  }

  /* Battery Strategy Advisory Banner */
  .solar-advisory {
    border-radius: 14px;
    padding: 10px 12px;
    margin-bottom: 12px;
    display: flex;
    flex-direction: column;
    gap: 4px;
    transition: all 0.2s ease;
  }
  .solar-advisory.sunny {
    background: linear-gradient(135deg, #ecfdf5 0%, #d1fae5 100%);
    border: 1px solid #a7f3d0;
    color: #065f46;
  }
  .solar-advisory.moderate {
    background: linear-gradient(135deg, #eff6ff 0%, #dbeafe 100%);
    border: 1px solid #bfdbfe;
    color: #1e40af;
  }
  .solar-advisory.rainy {
    background: linear-gradient(135deg, #fff7ed 0%, #fee2e2 100%);
    border: 1px solid #fecaca;
    color: #991b1b;
  }
  .solar-advisory-title {
    font-size: 0.82rem;
    font-weight: 800;
    display: flex;
    align-items: center;
    gap: 6px;
  }
  .solar-advisory-desc {
    font-size: 0.72rem;
    line-height: 1.35;
    font-weight: 500;
  }
  .solar-advisory-stats {
    display: flex;
    gap: 10px;
    font-size: 0.68rem;
    font-weight: 700;
    margin-top: 2px;
    opacity: 0.9;
    flex-wrap: wrap;
  }

  /* Summary Metrics Strip */
  .weather-summary-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 8px;
    margin-bottom: 12px;
  }
  .weather-summary-box {
    background: #f8fafc;
    border: 1px solid #e2e8f0;
    border-radius: 10px;
    padding: 7px 5px;
    text-align: center;
  }
  .weather-summary-label {
    font-size: 0.6rem;
    color: #64748b;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.4px;
  }
  .weather-summary-val {
    font-size: 0.85rem;
    font-weight: 800;
    color: #1e293b;
    margin-top: 2px;
  }

  /* 24h Timeline Scroll Area */
  .weather-timeline-container {
    position: relative;
    margin-top: 4px;
  }
  .weather-timeline-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 0.68rem;
    color: #475569;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    margin-bottom: 6px;
    padding: 0 2px;
  }
  .weather-timeline {
    display: flex;
    gap: 10px;
    overflow-x: auto;
    padding: 2px 2px 10px 2px;
    scroll-snap-type: x mandatory;
    -webkit-overflow-scrolling: touch;
    scrollbar-width: thin;
    scrollbar-color: #cbd5e1 transparent;
  }
  .weather-timeline::-webkit-scrollbar {
    height: 6px;
  }
  .weather-timeline::-webkit-scrollbar-thumb {
    background: #94a3b8;
    border-radius: 4px;
  }

  /* Broadcast TV Weather Hour Column (matching screenshot) */
  .weather-hour-col {
    flex: 0 0 84px;
    scroll-snap-align: start;
    background: linear-gradient(180deg, #d8eafb 0%, #b8d8f4 45%, #9ec5ec 100%);
    border: 1px solid rgba(255, 255, 255, 0.85);
    border-radius: 12px;
    display: flex;
    flex-direction: column;
    align-items: stretch;
    text-align: center;
    box-shadow: 0 4px 10px rgba(15, 23, 42, 0.12);
    overflow: hidden;
    position: relative;
    transition: transform 0.15s, box-shadow 0.15s;
  }
  .weather-hour-col:hover {
    transform: translateY(-2px);
    box-shadow: 0 6px 14px rgba(15, 23, 42, 0.18);
  }
  .weather-hour-col.now {
    border: 2px solid #2563eb;
    box-shadow: 0 0 0 2px rgba(37,99,235,0.25), 0 6px 14px rgba(15,23,42,0.18);
  }
  .weather-hour-col.night-col {
    background: linear-gradient(180deg, #c5d9ed 0%, #9cbcdb 50%, #7d9fc2 100%);
  }

  /* Deep Navy Cap (like 8 AM, 10 AM, 12 PM in screenshot) */
  .weather-col-header {
    background: #1c3a66;
    color: #ffffff;
    font-size: 0.74rem;
    font-weight: 800;
    padding: 5px 2px;
    letter-spacing: 0.5px;
    text-transform: uppercase;
  }
  .weather-hour-col.now .weather-col-header {
    background: #1d4ed8;
  }

  /* Column Inner Content */
  .weather-col-body {
    padding: 8px 4px 6px 4px;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    flex: 1;
  }
  .weather-col-icon {
    width: 44px;
    height: 44px;
    display: flex;
    align-items: center;
    justify-content: center;
    margin: 2px 0 4px 0;
  }
  
  /* Hero Temperature Number (large bold like 58, 72, 76, 81 in screenshot) */
  .weather-col-temp {
    font-size: 1.85rem;
    font-weight: 800;
    color: #122844;
    line-height: 1;
    letter-spacing: -0.5px;
    margin-bottom: 2px;
    display: flex;
    align-items: flex-start;
    justify-content: center;
  }
  .weather-col-temp .temp-deg {
    font-size: 1rem;
    font-weight: 700;
    margin-top: 1px;
    opacity: 0.85;
  }

  /* Wind / Solar Generation Subtitle (like E calm, SE 6 in screenshot) */
  .weather-col-sub {
    font-size: 0.64rem;
    font-weight: 700;
    color: #2c4c74;
    line-height: 1.2;
    margin-bottom: 4px;
    white-space: nowrap;
  }

  /* Bottom Precipitation Probability Bar (like 0% droplet in screenshot) */
  .weather-col-rain {
    background: rgba(22, 47, 85, 0.14);
    border-top: 1px solid rgba(22, 47, 85, 0.16);
    color: #162f55;
    font-size: 0.7rem;
    font-weight: 800;
    padding: 4px 2px;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 3px;
  }
  .weather-col-rain.rain-alert {
    background: #2563eb;
    color: #ffffff;
  }

  /* City Picker Modal Styles */
  .city-grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 6px;
    margin: 10px 0;
  }
  .btn-city-preset {
    background: #f8fafc;
    border: 1px solid #cbd5e1;
    border-radius: 8px;
    padding: 8px 6px;
    font-size: 0.75rem;
    font-weight: 600;
    color: #1e293b;
    cursor: pointer;
    text-align: left;
    display: flex;
    align-items: center;
    gap: 5px;
    transition: all 0.15s;
  }
  .btn-city-preset:hover, .btn-city-preset.active {
    border-color: #2563eb;
    background: #eff6ff;
    color: #1d4ed8;
  }
  .city-search-item {
    padding: 8px 10px;
    background: #f8fafc;
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    margin-bottom: 5px;
    font-size: 0.75rem;
    cursor: pointer;
    display: flex;
    justify-content: space-between;
    align-items: center;
    transition: all 0.15s;
  }
  .city-search-item:hover {
    background: #eff6ff;
    border-color: #3b82f6;
  }
</style>
</head>
<body>

<!-- Top Control & Card Alignment Toolbar -->
<div class="top-control-bar">
  <div class="top-brand">
    <span class="top-brand-icon">⚡</span>
    <span class="top-brand-text">Next 6.2kW Smart Solar</span>
  </div>
  <div class="layout-switch-bar">
    <span class="layout-switch-label">Align Cards:</span>
    <div class="layout-switch-buttons">
      <button class="btn-layout-opt active" id="btnLayoutAuto" onclick="setDashboardLayout('auto')" title="Auto responsive layout">🔄 Auto</button>
      <button class="btn-layout-opt" id="btnLayout1Col" onclick="setDashboardLayout('1col')" title="Single column stacked (clean & aligned)">📱 1-Col</button>
      <button class="btn-layout-opt" id="btnLayout2Col" onclick="setDashboardLayout('2col')" title="2-column side-by-side grid">🔲 2-Col</button>
      <button class="btn-layout-opt" id="btnLayout3Col" onclick="setDashboardLayout('3col')" title="3-column widescreen dashboard">🖥️ 3-Col</button>
    </div>
  </div>
</div>

<div class="page-wrapper" id="pageWrapper">

<!-- Card 1: Main Inverter Telemetry & Power Flow Diagram -->
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
    <div style="display:flex; flex-direction:column; align-items:flex-end; gap:5px;">
      <a href="/settings" style="text-decoration:none; background:#2563eb; color:#ffffff; font-size:0.74rem; font-weight:700; padding:6px 12px; border-radius:8px; display:inline-flex; align-items:center; gap:5px; box-shadow:0 2px 5px rgba(37,99,235,0.3); transition:all 0.15s;">⚙️ Settings & Diagnostics</a>
      <div id="dataFeedIndicator" style="font-size:0.68rem; color:#64748b; text-align:right;">No Serial Feed</div>
    </div>
  </div>

  <div class="diagram-container">
    <svg class="flow-svg" viewBox="0 0 360 390">
      <!-- 1. SOLAR LINE (Left Trunk x = 165) -->
      <path class="guide-path" d="M 114 70 H 151 Q 165 70 165 84 V 155" stroke="#fcd34d" stroke-dasharray="3 3" />
      <path id="pulseSolar" class="active-pulse" d="M 114 70 H 151 Q 165 70 165 84 V 155" stroke="#f59e0b" style="display:none;" />

      <!-- 2. GRID LINE (Right Trunk x = 195) -->
      <path class="guide-path" d="M 246 70 H 209 Q 195 70 195 84 V 155" stroke="#cbd5e1" stroke-dasharray="3 3" />
      <path id="pulseGrid" class="active-pulse" d="M 246 70 H 209 Q 195 70 195 84 V 155" stroke="#2563eb" style="display:none;" />

      <!-- 3. BATTERY LINE (Left Trunk x = 165) -->
      <path class="guide-path" d="M 165 235 V 284 Q 165 298 151 298 H 114" stroke="#a7f3d0" stroke-dasharray="3 3" />
      <path id="pulseBattery" class="active-pulse" d="M 165 235 V 284 Q 165 298 151 298 H 114" stroke="#10b981" style="display:none;" />

      <!-- 4. LOAD LINE (Right Trunk x = 195) -->
      <path class="guide-path" d="M 195 235 V 284 Q 195 298 209 298 H 246" stroke="#bfdbfe" stroke-dasharray="3 3" />
      <path id="pulseLoad" class="active-pulse" d="M 195 235 V 284 Q 195 298 209 298 H 246" stroke="#3b82f6" style="display:none;" />
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

  <!-- Live Serial Diagnostics & Raw Data -->
  <div class="diag-card">
    <div class="diag-row" style="margin-bottom:6px;">
      <span style="font-weight:700; color:#38bdf8; display:flex; align-items:center; gap:4px;">
        📡 Serial Diagnostics (RS-232)
      </span>
      <div style="display:flex; gap:4px;">
        <span id="diagBaudBadge" style="background:#1e293b; padding:2px 6px; border-radius:5px; color:#fbbf24; font-size:0.65rem;">2400 bps</span>
        <span id="diagPinBadge" style="background:#1e293b; padding:2px 6px; border-radius:5px; color:#a7f3d0; font-size:0.65rem;">RX:16 TX:17</span>
      </div>
    </div>

    <div class="diag-row" style="font-size:0.65rem; color:#94a3b8; margin-bottom:6px;">
      <span>TX Bytes: <b id="diagTxBytes" style="color:#e2e8f0;">0 B</b></span>
      <span>RX Bytes: <b id="diagRxBytes" style="color:#e2e8f0;">0 B</b></span>
    </div>

    <div style="margin-bottom:5px;">
      <span style="color:#64748b;">Last TX Command:</span>
      <b id="diagLastTx" style="color:#4ade80; margin-left:4px;">QPIGS</b>
    </div>

    <div style="margin-bottom:5px;">
      <span style="color:#64748b;">Last RX Raw String:</span>
      <div id="diagLastRx" class="diag-box" style="color:#38bdf8;">(Waiting for serial feed...)</div>
    </div>

    <div style="margin-bottom:6px;">
      <span style="color:#64748b;">Last RX Hex Dump:</span>
      <div id="diagLastHex" class="diag-box" style="color:#c084fc; font-size:0.62rem;">--</div>
    </div>

    <!-- Quick Control Buttons -->
    <div class="diag-btn-group">
      <button class="btn-diag" onclick="setBaud(2400)">2400</button>
      <button class="btn-diag" onclick="setBaud(9600)">9600</button>
      <button class="btn-diag" style="background:#047857;" onclick="swapPins()">🔀 Swap RX/TX</button>
      <button class="btn-diag" style="flex:1;" onclick="sendManualCmd('QPIGS')">Send QPIGS</button>
      <button class="btn-diag" onclick="sendManualCmd('QMOD')">QMOD</button>
      <button class="btn-diag" onclick="sendManualCmd('QPI')">QPI</button>
    </div>
  </div>

  <!-- Bottom Wi-Fi & Settings Bar -->
  <div class="bottom-wifi">
    <div class="wifi-indicator">
      <span id="wifiDot" class="dot-green"></span>
      <span id="wifiLabel">Connecting to Wi-Fi...</span>
    </div>
    <div style="display:flex; gap:6px;">
      <a href="/settings" style="text-decoration:none; background:#0f172a; color:#ffffff; font-size:0.74rem; font-weight:700; padding:6px 10px; border-radius:8px; display:inline-flex; align-items:center; gap:4px;">⚙️ Live Parameters</a>
      <button class="btn-connect" onclick="openWifiModal()">📶 Wi-Fi</button>
    </div>
  </div>

  <!-- Firmware Version Footer -->
  <div style="display:flex; justify-content:space-between; align-items:center; font-size:0.7rem; color:#64748b; margin-top:8px; padding:0 6px;">
    <span>NEXT Inverter Wi-Fi Card</span>
    <span>Firmware: <b id="footerVersion" style="color:#0f172a;">v1.1.6</b></span>
  </div>

</div> <!-- /Card 1: app-frame -->

<!-- Column 2 / Center Stack: JK-BMS Live Monitor + Inverter Mode Macros -->
<div class="dashboard-col">

<!-- Card 2: 14S Nissan Leaf — JK-BMS Live Monitor -->
<div class="bms-card">
  <div class="bms-card-header">
    <div>
      <div class="bms-title">
        🔋 14S Nissan Leaf — JK-BMS Monitor
        <span id="bmsHeaderVolt" class="bms-volt-badge" style="display:none;">-- V</span>
      </div>
      <div id="bmsStatusSub" class="bms-subtitle">Connecting via Bluetooth BLE...</div>
    </div>
    <div class="bms-header-actions">
      <button id="btnBmsOn" class="btn-bms-action btn-bms-on" onclick="setBmsBluetooth(true)" title="Turn ON Bluetooth connection to JK-BMS">🔵 Turn On</button>
      <button id="btnBmsOff" class="btn-bms-action btn-bms-off" onclick="setBmsBluetooth(false)" title="Turn OFF Bluetooth connection">⚪ Turn Off</button>
      <button class="btn-setup-bms" onclick="openBmsModal()" title="Setup BMS">⚙️ Setup</button>
    </div>
  </div>

  <!-- Key Metrics Row: Battery Total Voltage + Delta V + 3 Temperatures -->
  <div class="bms-metrics-row">
    <!-- Battery Total Pack Voltage Pill -->
    <div class="bms-metric-box bms-volt-box" id="bmsVoltBox">
      <div class="bms-metric-label">BATTERY VOLTS</div>
      <div class="bms-metric-val" id="bmsPackVolt">-- <span style="font-size:0.75rem;">V</span></div>
      <div class="bms-metric-sub" id="bmsAvgCellSub">14S Pack</div>
    </div>

    <!-- Delta V Metric Pill -->
    <div class="bms-metric-box bms-delta-box" id="bmsDeltaBox">
      <div class="bms-metric-label">VOLTAGE DIFF</div>
      <div class="bms-metric-val" id="bmsDeltaVal">-- <span style="font-size:0.75rem;">mV</span></div>
      <div class="bms-metric-sub" id="bmsDeltaSub">Delta V</div>
    </div>

    <!-- 3 Temperature Sensors -->
    <div class="bms-metric-box">
      <div class="bms-metric-label">BATT TEMP 1</div>
      <div class="bms-metric-val" id="bmsTemp1">-- <span style="font-size:0.75rem;">°C</span></div>
      <div class="bms-metric-sub">Probe T1</div>
    </div>
    <div class="bms-metric-box">
      <div class="bms-metric-label">BATT TEMP 2</div>
      <div class="bms-metric-val" id="bmsTemp2">-- <span style="font-size:0.75rem;">°C</span></div>
      <div class="bms-metric-sub">Probe T2</div>
    </div>
    <div class="bms-metric-box">
      <div class="bms-metric-label">MOS TEMP</div>
      <div class="bms-metric-val" id="bmsTempMos">-- <span style="font-size:0.75rem;">°C</span></div>
      <div class="bms-metric-sub">Power Board</div>
    </div>
  </div>

  <!-- 14 Cells Header -->
  <div class="bms-cells-title">
    <span>14 Cell Voltages</span>
    <span style="font-size:0.68rem; color:#64748b; font-weight:700;">
      <span style="display:inline-block; width:8px; height:8px; border-radius:2px; background:#2563eb; margin-right:3px;"></span>Highest
      <span style="display:inline-block; width:8px; height:8px; border-radius:2px; background:#dc2626; margin-left:8px; margin-right:3px;"></span>Lowest
    </span>
  </div>

  <!-- 14 Cells Grid (Highest=Blue, Lowest=Red) -->
  <div class="bms-cells-grid" id="bmsCellsGrid">
    <!-- Populated by renderBmsData() -->
  </div>
</div>

<!-- Card 3: Standalone Inverter Mode Macros Card -->
<div class="macro-card">
  <div class="macro-card-header">
    <div>
      <div class="macro-title">⚡ Inverter Mode Macros</div>
      <div class="macro-subtitle">1-Click Custom Presets (RS-232)</div>
    </div>
    <div style="display:flex; gap:6px;">
      <a href="/settings" style="text-decoration:none; background:#f8fafc; color:#1e293b; border:1px solid #cbd5e1; font-size:0.72rem; font-weight:700; padding:5px 8px; border-radius:8px; display:inline-flex; align-items:center; gap:4px;">⚙️ Inverter Programs</a>
      <button class="btn-create-macro" onclick="openCreateMacroModal()">+ Create Macro</button>
    </div>
  </div>

  <!-- Real-time Macro Execution Feedback & Error Details Banner -->
  <div id="macroStatusBanner" class="macro-status-banner"></div>

  <!-- Macro Buttons Container -->
  <div id="macrosContainer" class="macros-list">
    <div style="text-align:center; padding:12px; color:#64748b; font-size:0.75rem;">
      Loading macros...
    </div>
  </div>
</div>

</div> <!-- /dashboard-col -->

<!-- Card 3: 24h Solar & Weather Forecast Card (Broadcast TV Graphic Theme) -->
<div class="weather-card">
  <!-- TV Broadcast Style Header Bar -->
  <div class="weather-broadcast-header">
    <div class="broadcast-station-info">
      <div class="broadcast-channel-title">
        <span class="broadcast-live-badge">LIVE</span>
        <span>SOLAR WEATHER RADAR</span>
      </div>
      <div class="broadcast-timeline-sub" id="weatherSubtitle">
        <span id="weatherUpdatedTime">24-Hour Satellite Prediction</span>
      </div>
    </div>
    
    <!-- TV Two-tone Location Pill (matching screenshot) -->
    <div class="broadcast-city-badge" onclick="openCityModal()" title="Click to change location">
      <div class="city-name-box" id="weatherCityName">COLOMBO</div>
      <div class="city-tag-box">CITY 📍</div>
    </div>
  </div>

  <!-- Solar Strategy / Battery Decision Banner -->
  <div id="solarAdvisoryBanner" class="solar-advisory sunny">
    <div class="solar-advisory-title" id="advisoryTitle">
      <span>☀️ Checking solar forecast...</span>
    </div>
    <div class="solar-advisory-desc" id="advisoryDesc">
      Fetching 24-hour satellite forecast to calculate optimal battery mode...
    </div>
    <div class="solar-advisory-stats" id="advisoryStats"></div>
  </div>

  <!-- Summary Metric Strip -->
  <div class="weather-summary-grid">
    <div class="weather-summary-box">
      <div class="weather-summary-label">Tomorrow Sun</div>
      <div class="weather-summary-val" id="summarySolar">--</div>
    </div>
    <div class="weather-summary-box">
      <div class="weather-summary-label">Max Rain Risk</div>
      <div class="weather-summary-val" id="summaryRain">--</div>
    </div>
    <div class="weather-summary-box">
      <div class="weather-summary-label">Avg Cloud Cover</div>
      <div class="weather-summary-val" id="summaryCloud">--</div>
    </div>
  </div>

  <!-- Hourly Timeline -->
  <div class="weather-timeline-container">
    <div class="weather-timeline-header">
      <span>Hourly Timeline (Next 24 Hours)</span>
      <span style="font-size:0.62rem; color:#94a3b8;">Swipe ↔</span>
    </div>
    <div id="weatherTimeline" class="weather-timeline">
      <div style="text-align:center; width:100%; padding:16px 0; color:#64748b; font-size:0.75rem;">
        Fetching hourly satellite forecast...
      </div>
    </div>
  </div>
</div>

</div> <!-- /page-wrapper -->

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

    <!-- ThingsBoard Cloud Integration Section -->
    <div class="section-divider"></div>
    <div class="section-title" style="display:flex; justify-content:space-between; align-items:center;">
      <span>ThingsBoard Cloud History</span>
      <span id="tbStatusPill" style="font-size:0.65rem; padding:2px 6px; border-radius:4px; background:#e2e8f0; color:#475569;">Disabled</span>
    </div>
    
    <div style="margin-top:6px; display:flex; align-items:center; gap:8px;">
      <input type="checkbox" id="tbEnabled" style="width:16px; height:16px; margin:0;">
      <label for="tbEnabled" style="font-size:0.75rem; font-weight:600; color:#1e293b; cursor:pointer;">Enable ThingsBoard Cloud Logging</label>
    </div>

    <div style="margin-top:6px;">
      <label style="font-size:0.72rem; color:#475569;">Server URL:</label>
      <input type="text" id="tbServer" placeholder="https://thingsboard.cloud" value="https://thingsboard.cloud">
    </div>

    <div style="margin-top:4px;">
      <label style="font-size:0.72rem; color:#475569;">Device Access Token:</label>
      <input type="text" id="tbToken" placeholder="Paste ThingsBoard Device Token">
    </div>

    <div style="margin-top:4px; display:flex; justify-content:space-between; align-items:center;">
      <label style="font-size:0.72rem; color:#475569;">Averaging Interval:</label>
      <select id="tbInterval" style="width:160px; padding:6px; font-size:0.75rem;">
        <option value="10" selected>10s (~3 samples avg)</option>
        <option value="15">15s (~5 samples avg)</option>
        <option value="30">30s (~10 samples avg)</option>
        <option value="60">60s (~20 samples avg)</option>
      </select>
    </div>

    <button class="btn-save" style="background:#2563eb; width:100%; margin-top:8px;" onclick="saveTbConfig()">💾 Save ThingsBoard Settings</button>

    <!-- Blynk IoT Cloud Integration Section -->
    <div class="section-divider"></div>
    <div class="section-title" style="display:flex; justify-content:space-between; align-items:center;">
      <span>Blynk IoT Cloud (Phone App)</span>
      <span id="blynkStatusPill" style="font-size:0.65rem; padding:2px 6px; border-radius:4px; background:#e2e8f0; color:#475569;">Disabled</span>
    </div>

    <div style="margin-top:6px; display:flex; align-items:center; gap:8px;">
      <input type="checkbox" id="blynkEnabled" style="width:16px; height:16px; margin:0;">
      <label for="blynkEnabled" style="font-size:0.75rem; font-weight:600; color:#1e293b; cursor:pointer;">Enable Blynk Cloud Streaming</label>
    </div>

    <div style="margin-top:6px;">
      <label style="font-size:0.72rem; color:#475569;">Blynk Server URL:</label>
      <input type="text" id="blynkServer" placeholder="https://blynk.cloud" value="https://blynk.cloud">
    </div>

    <div style="margin-top:4px;">
      <label style="font-size:0.72rem; color:#475569;">Device Auth Token:</label>
      <input type="text" id="blynkToken" placeholder="Paste Blynk Device Auth Token">
    </div>

    <div style="margin-top:4px; display:flex; justify-content:space-between; align-items:center;">
      <label style="font-size:0.72rem; color:#475569;">Update Interval:</label>
      <select id="blynkInterval" style="width:160px; padding:6px; font-size:0.75rem;">
        <option value="10" selected>10s (Fast)</option>
        <option value="15">15s (Balanced)</option>
        <option value="30">30s (Eco)</option>
        <option value="60">60s (1 min)</option>
      </select>
    </div>

    <button class="btn-save" style="background:#059669; width:100%; margin-top:8px;" onclick="saveBlynkConfig()">📱 Save Blynk Settings</button>

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

<!-- Create / Edit Macro Preset Modal -->
<div id="macroModal" class="modal-overlay">
  <div class="modal" style="max-width:440px;">
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:8px;">
      <h2 id="macroModalTitle" style="display:flex; align-items:center; gap:6px;">⚡ Create Inverter Macro</h2>
      <button class="btn-cancel" onclick="closeMacroModal()" style="font-size:0.75rem; padding:4px 8px;">✕</button>
    </div>
    <p style="font-size:0.73rem; color:#64748b; margin-bottom:12px; line-height:1.35;">
      Create a 1-click preset button. Choose which settings to change on the inverter over RS-232. Leave unneeded fields unchanged.
    </p>

    <input type="hidden" id="editMacroId" value="">

    <!-- Macro Name -->
    <div class="section-title">Macro Button Name</div>
    <input type="text" id="macroInputName" placeholder="e.g. Day Solar Power, Night Battery, Grid Fast Charge" maxlength="45" style="margin-bottom:10px;">

    <!-- Program 01: Output Source Priority -->
    <div class="section-title">Program 01: Output Source Priority</div>
    <select id="macroOutPriority" style="margin-bottom:10px;">
      <option value="">-- Leave Unchanged --</option>
      <option value="POP02">Solar First, Battery Second, Utility Grid Last (SBU Priority - Recommended for Leaf & Solar)</option>
      <option value="POP01">Solar First, Utility Grid Blending & Assistance (SUB Mode - Blends Grid when load is high)</option>
      <option value="POP00">Utility Grid First (USB Mode - Utility grid powers load, Solar charges battery)</option>
    </select>

    <!-- Program 16: Charger Source Priority -->
    <div class="section-title">Program 16: Charger Source Priority</div>
    <select id="macroChgPriority" style="margin-bottom:10px;">
      <option value="">-- Leave Unchanged --</option>
      <option value="PCP02">Only Solar Charging (OSO - 100% Free Solar, strictly no Grid charging)</option>
      <option value="PCP01">Solar & Utility Charging (SNU - Simultaneous Solar & Grid charging)</option>
      <option value="PCP03">Solar Energy First (CSO - Solar charges battery first; Utility only if no solar)</option>
      <option value="PCP00">Utility First Charging</option>
    </select>

    <!-- Program 02: Maximum Charging Current (Capped at 30A for safety) -->
    <div class="section-title">Program 02: Maximum Total Charging Current</div>
    <select id="macroMaxChgCurr" style="margin-bottom:10px;">
      <option value="">-- Leave Unchanged --</option>
      <option value="MNCHGC010">10 Amps (~500 Watts - Ultra gentle trickle charge)</option>
      <option value="MNCHGC020">20 Amps (~1000 Watts - Safe standard rate for 50Ah Leaf battery)</option>
      <option value="MNCHGC030">30 Amps (~1500 Watts - Max Safe Rate for 50Ah Leaf battery)</option>
    </select>

    <!-- Program 11: Maximum Utility Grid Charging Current (Capped at 30A for safety) -->
    <div class="section-title">Program 11: Maximum Utility Grid Charging Current</div>
    <select id="macroMaxUtilCurr" style="margin-bottom:6px;">
      <option value="">-- Leave Unchanged --</option>
      <option value="MUCHGC002">2 Amps (Zero / Minimal Grid Charging - Recommended for OSO)</option>
      <option value="MUCHGC010">10 Amps (~500 Watts - Gentle night grid charge)</option>
      <option value="MUCHGC020">20 Amps (~1000 Watts - Standard grid charging)</option>
      <option value="MUCHGC030">30 Amps (~1500 Watts - Max Safe grid charging)</option>
    </select>
    <div style="font-size:0.68rem; color:#059669; font-weight:600; margin-bottom:10px;">💡 Tip: Set to 2A when using USB Mode + OSO to strictly stop grid power from charging your battery.</div>

    <!-- Program 23: Overload Bypass to Grid -->
    <div class="section-title">Program 23: Overload Bypass to Grid</div>
    <select id="macroOverloadBypass" style="margin-bottom:10px;">
      <option value="">-- Leave Unchanged --</option>
      <option value="PEb">Enable Overload Bypass (Automatically transfer load to Grid if inverter is overloaded)</option>
      <option value="PDb">Disable Overload Bypass (Do not transfer to grid on overload)</option>
    </select>

    <!-- Battery Cycling Thresholds Collapsible Section -->
    <div class="section-divider"></div>
    <details style="margin-bottom:10px; font-size:0.8rem;">
      <summary style="font-weight:700; color:#334155; cursor:pointer; margin-bottom:6px; outline:none;">
        ⚙️ Optional: Grid Transfer Voltages (SBU / SUB)
      </summary>
      <div style="padding-top:6px; display:flex; flex-direction:column; gap:4px;">
        <label style="font-size:0.69rem; color:#64748b; font-weight:600;">Program 12: Back to Utility Grid Voltage in SBU (42.0V to 51.0V)</label>
        <input type="number" step="0.1" min="42.0" max="51.0" id="macroBackGridVolt" placeholder="e.g. 48.0 (Leave blank if unchanged)">

        <label style="font-size:0.69rem; color:#64748b; font-weight:600;">Program 13: Back to Battery Discharge Voltage in SBU (48.0V to 58.0V)</label>
        <input type="number" step="0.1" min="48.0" max="58.0" id="macroBackBatVolt" placeholder="e.g. 51.0 (Leave blank if unchanged)">
      </div>
    </details>

    <div id="macroModalMsg" style="font-size:0.75rem; font-weight:600; margin-bottom:6px;"></div>

    <div class="modal-buttons">
      <button type="button" class="btn-cancel" onclick="closeMacroModal()">Cancel</button>
      <button type="button" class="btn-save" onclick="saveMacroPreset()">Save Macro Button</button>
    </div>
  </div>
</div>

<!-- City Selection Modal -->
<div id="cityModal" class="modal-overlay">
  <div class="modal" style="max-width:400px;">
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:8px;">
      <h2 style="display:flex; align-items:center; gap:6px;">📍 Select Location</h2>
      <button class="btn-cancel" onclick="closeCityModal()" style="font-size:0.75rem; padding:4px 8px;">✕</button>
    </div>
    <p style="font-size:0.73rem; color:#64748b; margin-bottom:10px;">
      Select your location to get accurate satellite solar irradiance & weather predictions.
    </p>

    <!-- Quick Presets -->
    <div class="section-title">Common Sri Lankan Cities</div>
    <div class="city-grid">
      <button class="btn-city-preset" onclick="selectCity('Colombo', 6.9271, 79.8612)">📍 Colombo</button>
      <button class="btn-city-preset" onclick="selectCity('Gampaha', 7.0840, 79.9939)">📍 Gampaha</button>
      <button class="btn-city-preset" onclick="selectCity('Negombo', 7.2088, 79.8358)">📍 Negombo</button>
      <button class="btn-city-preset" onclick="selectCity('Kandy', 7.2906, 80.6337)">📍 Kandy</button>
      <button class="btn-city-preset" onclick="selectCity('Kurunegala', 7.4863, 80.3623)">📍 Kurunegala</button>
      <button class="btn-city-preset" onclick="selectCity('Galle', 6.0535, 80.2210)">📍 Galle</button>
      <button class="btn-city-preset" onclick="selectCity('Matara', 5.9549, 80.5550)">📍 Matara</button>
      <button class="btn-city-preset" onclick="selectCity('Jaffna', 9.6615, 80.0255)">📍 Jaffna</button>
      <button class="btn-city-preset" onclick="selectCity('Anuradhapura', 8.3114, 80.4037)">📍 Anuradhapura</button>
      <button class="btn-city-preset" onclick="selectCity('Batticaloa', 7.7310, 81.6747)">📍 Batticaloa</button>
    </div>

    <!-- Custom Search -->
    <div class="section-divider"></div>
    <div class="section-title">Search Any Other City / Town</div>
    <div style="display:flex; gap:6px;">
      <input type="text" id="citySearchInput" placeholder="e.g. Kalutara, Ratnapura, Badulla..." style="margin:0;" onkeydown="if(event.key==='Enter') searchCustomCity()">
      <button type="button" class="btn-save" style="padding:7px 12px; white-space:nowrap;" onclick="searchCustomCity()">Search</button>
    </div>
    <div id="citySearchResults" style="margin-top:8px; max-height:140px; overflow-y:auto;"></div>

    <div class="modal-buttons" style="margin-top:14px;">
      <button type="button" class="btn-cancel" onclick="closeCityModal()">Close</button>
    </div>
  </div>
</div>

<!-- JK-BMS Bluetooth Setup Modal -->
<div id="bmsModal" class="modal-overlay">
  <div class="modal" style="max-width:440px;">
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:8px;">
      <h2 style="display:flex; align-items:center; gap:6px;">📡 JK-BMS Bluetooth Setup</h2>
      <button class="btn-cancel" onclick="closeBmsModal()" style="font-size:0.75rem; padding:4px 8px;">✕</button>
    </div>
    <p style="font-size:0.73rem; color:#64748b; margin-bottom:12px; line-height:1.35;">
      Search for your JK Smart BMS over Bluetooth Low Energy (BLE), select your device, and enter your BMS PIN password.
    </p>

    <div style="margin-bottom:10px; display:flex; align-items:center; gap:8px;">
      <input type="checkbox" id="bmsEnabled" style="width:16px; height:16px; margin:0;">
      <label for="bmsEnabled" style="font-size:0.78rem; font-weight:700; color:#1e293b; cursor:pointer;">Enable JK-BMS Bluetooth Monitoring</label>
    </div>

    <!-- Bluetooth Scanner -->
    <div class="section-title" style="display:flex; justify-content:space-between; align-items:center;">
      <span>Nearby Bluetooth Devices</span>
      <button type="button" class="btn-cancel" id="btnBmsScan" onclick="scanBmsDevices()" style="font-size:0.7rem; padding:3px 8px;">🔍 Search Devices</button>
    </div>
    <div id="bmsScanStatus" style="font-size:0.72rem; color:#64748b; margin-bottom:4px;">Click "Search Devices" to scan nearby BLE devices.</div>

    <select id="bmsDeviceSelect" onchange="onBmsDeviceSelected()" style="margin-bottom:8px;">
      <option value="" disabled selected>-- Select Discovered BMS --</option>
    </select>

    <div style="display:flex; gap:6px; margin-bottom:8px;">
      <div style="flex:1;">
        <label style="font-size:0.7rem; color:#475569; font-weight:600;">Device Name:</label>
        <input type="text" id="bmsDeviceName" placeholder="e.g. JK-B2A8S20P" style="margin-top:2px;">
      </div>
      <div style="flex:1.2;">
        <label style="font-size:0.7rem; color:#475569; font-weight:600;">Bluetooth MAC Address:</label>
        <input type="text" id="bmsDeviceMac" placeholder="XX:XX:XX:XX:XX:XX" style="margin-top:2px;">
      </div>
    </div>

    <!-- Password / PIN -->
    <div class="section-title">BMS PIN / Password</div>
    <input type="password" id="bmsPin" placeholder="Default is 1234 (or 123456)" value="1234" maxlength="16" style="margin-bottom:6px;">
    <div style="font-size:0.68rem; color:#64748b; margin-bottom:12px;">Standard factory PIN is <code>1234</code>. Enter your custom PIN if changed in the JK app.</div>

    <!-- Live BLE Diagnostics Status Box -->
    <div id="bmsDiagInfo" style="display:none; margin-bottom:10px; padding:8px 10px; background:#f1f5f9; border:1px solid #cbd5e1; border-radius:8px; font-size:0.68rem; color:#334155; line-height:1.45;"></div>

    <div id="bmsModalMsg" style="font-size:0.75rem; font-weight:600; margin-bottom:8px;"></div>

    <div class="modal-buttons">
      <button type="button" class="btn-cancel" onclick="closeBmsModal()">Cancel</button>
      <button type="button" class="btn-save" onclick="saveBmsConfig()">💾 Save & Connect BMS</button>
    </div>
  </div>
</div>

<script>
let isMacroApplying = false;
async function refreshData() {
  if (isMacroApplying) return; // Prevent background polling collisions during macro runs
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
    const pulseSolar = document.getElementById('pulseSolar');
    if (pulseSolar) pulseSolar.style.display = (pvW > 10) ? 'block' : 'none';

    // Grid
    const gridW = Math.round(d.grid_watts !== undefined ? d.grid_watts : 0);
    document.getElementById('gridWatts').innerHTML = gridW + '<span style="font-size:0.9rem;">w</span>';
    const gridA = (d.grid_voltage > 0 ? (gridW / d.grid_voltage).toFixed(2) : "0.00");
    document.getElementById('gridSub').textContent = d.grid_voltage.toFixed(1) + "V • " + d.grid_frequency.toFixed(1) + "Hz";
    const pulseGrid = document.getElementById('pulseGrid');
    if (pulseGrid) pulseGrid.style.display = (gridW > 10) ? 'block' : 'none';

    // Inverter
    document.getElementById('invTemp').textContent = d.heatsink_temp.toFixed(1) + " °C";

    // Battery
    const batW = Math.abs(Math.round(d.battery_power_watts));
    document.getElementById('batHero').innerHTML = batW + '<span style="font-size:0.85rem;">w</span> • ' + d.battery_soc + '<span style="font-size:0.85rem;">%</span>';
    const batA = (d.battery_charge_current > 0 ? d.battery_charge_current : d.battery_discharge_current);
    document.getElementById('batSub').textContent = d.battery_voltage.toFixed(1) + "V • " + batA + "A";
    
    const pulseBat = document.getElementById('pulseBattery');
    if (d.battery_charge_current > 0) {
      document.getElementById('batCaption').innerHTML = 'Charging <span class="help-icon">?</span>';
      if (pulseBat) {
        pulseBat.style.display = 'block';
        pulseBat.classList.remove('reverse'); // Inverter -> Battery (Charging)
      }
    } else if (d.battery_discharge_current > 0) {
      document.getElementById('batCaption').innerHTML = 'Discharging <span class="help-icon">?</span>';
      if (pulseBat) {
        pulseBat.style.display = 'block';
        pulseBat.classList.add('reverse'); // Battery -> Inverter (Discharging)
      }
    } else {
      document.getElementById('batCaption').innerHTML = 'Standby <span class="help-icon">?</span>';
      if (pulseBat) {
        pulseBat.style.display = 'none';
        pulseBat.classList.remove('reverse');
      }
    }

    // Load
    const loadW = d.ac_output_watts;
    document.getElementById('loadWatts').innerHTML = loadW + '<span style="font-size:0.9rem;">w</span>';
    const loadA = (d.ac_output_voltage > 0 ? (loadW / d.ac_output_voltage).toFixed(2) : "0.00");
    document.getElementById('loadSub').textContent = d.ac_output_voltage.toFixed(1) + "V • " + loadA + "A";
    const pulseLoad = document.getElementById('pulseLoad');
    if (pulseLoad) pulseLoad.style.display = (loadW > 10) ? 'block' : 'none';

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
    const footVer = document.getElementById('footerVersion');
    if (footVer) footVer.textContent = 'v' + d.firmware_version;
    if (d.github_repo && !document.getElementById('githubRepoInput').value) {
      document.getElementById('githubRepoInput').value = d.github_repo;
    }

    // Serial Diagnostics
    if (d.last_tx_cmd) document.getElementById('diagLastTx').textContent = d.last_tx_cmd;
    if (d.last_rx_raw !== undefined) document.getElementById('diagLastRx').textContent = d.last_rx_raw;
    if (d.last_rx_hex !== undefined) document.getElementById('diagLastHex').textContent = d.last_rx_hex;
    if (d.rx_byte_count !== undefined) document.getElementById('diagRxBytes').textContent = d.rx_byte_count + ' B';
    if (d.tx_byte_count !== undefined) document.getElementById('diagTxBytes').textContent = d.tx_byte_count + ' B';
    if (d.current_baud !== undefined) document.getElementById('diagBaudBadge').textContent = d.current_baud + ' bps';
    if (d.rx_pin !== undefined && d.tx_pin !== undefined) {
      document.getElementById('diagPinBadge').textContent = (d.pins_swapped ? '🔀 ' : '') + 'RX:' + d.rx_pin + ' TX:' + d.tx_pin;
    }

    // ThingsBoard Status
    if (d.tb_enabled !== undefined) {
      updateTbStatus(d.tb_enabled, d.tb_status);
    }

    // Blynk IoT Status
    if (d.blynk_enabled !== undefined) {
      updateBlynkStatus(d.blynk_enabled, d.blynk_status);
    }

    // JK-BMS 14S Bluetooth Telemetry
    if (d.bms) {
      renderBmsData(d.bms);
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

// -------------------------------------------------------------
// JK-BMS 14S Nissan Leaf Renderer & Setup Modal Controller
// -------------------------------------------------------------
function renderBmsData(bms) {
  if (!bms) return;
  const statusSub = document.getElementById('bmsStatusSub');
  const btnOn = document.getElementById('btnBmsOn');
  const btnOff = document.getElementById('btnBmsOff');
  if (btnOn && btnOff) {
    if (bms.enabled) {
      btnOn.classList.add('active');
      btnOff.classList.remove('active');
    } else {
      btnOn.classList.remove('active');
      btnOff.classList.add('active');
    }
  }

  if (bms.connected && bms.valid) {
    statusSub.innerHTML = `<span style="color:#059669; font-weight:700;">🟢 Connected: ${escapeHtml(bms.name || 'JK-BMS')}</span> (RSSI: ${bms.rssi} dBm${bms.balancing ? ' • ⚖️ Balancing' : ''})`;
  } else if (bms.connected) {
    const rxInfo = (bms.rx_packets !== undefined) ? ` • Rx: ${bms.rx_packets} pkts, Tx: ${bms.tx_cmds || 0}` : '';
    statusSub.innerHTML = `<span style="color:#2563eb; font-weight:700;">🔵 Connected</span> (Waiting for telemetry stream${rxInfo}...)`;
  } else if (bms.enabled) {
    statusSub.innerHTML = `<span style="color:#d97706; font-weight:700;">🟡 ${escapeHtml(bms.status || 'Connecting...')}</span>`;
  } else {
    statusSub.innerHTML = `<span style="color:#64748b;">⚪ Bluetooth Turned OFF (Click "🔵 Turn On" to connect)</span>`;
  }

  // Battery Total Pack Voltage Pill & Header Badge
  const voltVal = document.getElementById('bmsPackVolt');
  const avgSub = document.getElementById('bmsAvgCellSub');
  const headerVolt = document.getElementById('bmsHeaderVolt');
  if (bms.valid && bms.pack_voltage !== undefined && bms.pack_voltage > 0) {
    const vStr = bms.pack_voltage.toFixed(2);
    if (voltVal) voltVal.innerHTML = `${vStr} <span style="font-size:0.75rem;">V</span>`;
    if (avgSub) avgSub.textContent = bms.avg_cell_v ? `Avg: ${bms.avg_cell_v.toFixed(3)}V` : '14S Pack';
    if (headerVolt) {
      headerVolt.textContent = `${vStr} V`;
      headerVolt.style.display = 'inline-block';
    }
  } else {
    if (voltVal) voltVal.innerHTML = `-- <span style="font-size:0.75rem;">V</span>`;
    if (avgSub) avgSub.textContent = '14S Pack';
    if (headerVolt) headerVolt.style.display = 'none';
  }

  // Delta V Pill
  const deltaBox = document.getElementById('bmsDeltaBox');
  const deltaVal = document.getElementById('bmsDeltaVal');
  const deltaSub = document.getElementById('bmsDeltaSub');
  if (bms.valid && bms.delta_mv !== undefined) {
    const dMv = Math.round(bms.delta_mv);
    deltaVal.innerHTML = `${dMv} <span style="font-size:0.75rem;">mV</span>`;
    deltaBox.className = 'bms-metric-box bms-delta-box ' + (dMv <= 15 ? 'balanced' : (dMv <= 30 ? 'moderate' : 'imbalanced'));
    deltaSub.textContent = dMv <= 15 ? 'Balanced 🟢' : (dMv <= 30 ? 'Moderate 🟡' : 'Imbalanced 🔴');
  } else {
    deltaVal.innerHTML = `-- <span style="font-size:0.75rem;">mV</span>`;
    deltaBox.className = 'bms-metric-box bms-delta-box';
    deltaSub.textContent = 'Delta V';
  }

  // 3 Temperatures
  document.getElementById('bmsTemp1').innerHTML = (bms.valid && bms.temp_sensor1 !== undefined) ? `${bms.temp_sensor1.toFixed(1)} <span style="font-size:0.75rem;">°C</span>` : `-- <span style="font-size:0.75rem;">°C</span>`;
  document.getElementById('bmsTemp2').innerHTML = (bms.valid && bms.temp_sensor2 !== undefined) ? `${bms.temp_sensor2.toFixed(1)} <span style="font-size:0.75rem;">°C</span>` : `-- <span style="font-size:0.75rem;">°C</span>`;
  document.getElementById('bmsTempMos').innerHTML = (bms.valid && bms.temp_mos !== undefined) ? `${bms.temp_mos.toFixed(1)} <span style="font-size:0.75rem;">°C</span>` : `-- <span style="font-size:0.75rem;">°C</span>`;

  // Render 14 Cell Boxes
  const grid = document.getElementById('bmsCellsGrid');
  if (!bms.valid || !Array.isArray(bms.cells) || bms.cells.length === 0) {
    let emptyHtml = '';
    for (let i = 1; i <= 14; i++) {
      emptyHtml += `
        <div class="bms-cell-box">
          <div class="bms-cell-label">C${i < 10 ? '0' + i : i}</div>
          <div class="bms-cell-volt">--<span style="font-size:0.56rem; font-weight:600; opacity:0.8;">V</span></div>
          <div class="bms-cell-bar-bg"><div class="bms-cell-bar-fill" style="width:0%;"></div></div>
        </div>`;
    }
    grid.innerHTML = emptyHtml;
    return;
  }

  let html = '';
  const cellCount = Math.min(14, bms.cells.length);
  const maxIdx = bms.max_cell_idx; // 1-indexed (1 to 14)
  const minIdx = bms.min_cell_idx; // 1-indexed (1 to 14)

  for (let i = 0; i < cellCount; i++) {
    const cellNum = i + 1;
    const v = bms.cells[i];
    const isHighest = (cellNum === maxIdx);
    const isLowest = (cellNum === minIdx);

    // Percentage of Li-ion range (3.2V to 4.15V)
    const pct = Math.max(0, Math.min(100, Math.round(((v - 3.20) / (4.15 - 3.20)) * 100)));

    let boxClass = 'bms-cell-box';
    let badgeHtml = '';
    if (isHighest) {
      boxClass += ' highest';
      badgeHtml = `<span class="bms-cell-badge max">MAX</span>`;
    } else if (isLowest) {
      boxClass += ' lowest';
      badgeHtml = `<span class="bms-cell-badge min">MIN</span>`;
    }

    html += `
      <div class="${boxClass}">
        ${badgeHtml}
        <div class="bms-cell-label">C${cellNum < 10 ? '0' + cellNum : cellNum}</div>
        <div class="bms-cell-volt">${v.toFixed(3)}<span style="font-size:0.56rem; font-weight:600; opacity:0.8;">V</span></div>
        <div class="bms-cell-bar-bg">
          <div class="bms-cell-bar-fill" style="width:${pct}%;"></div>
        </div>
      </div>
    `;
  }
  grid.innerHTML = html;
}

// -------------------------------------------------------------
// JK-BMS Bluetooth Quick Actions (Turn On / Turn Off)
// -------------------------------------------------------------
async function setBmsBluetooth(enable) {
  const btnOn = document.getElementById('btnBmsOn');
  const btnOff = document.getElementById('btnBmsOff');
  const statusSub = document.getElementById('bmsStatusSub');

  if (btnOn) btnOn.disabled = true;
  if (btnOff) btnOff.disabled = true;
  if (statusSub) {
    statusSub.innerHTML = `<span style="color:#2563eb; font-weight:700;">⏳ Turning Bluetooth ${enable ? 'ON' : 'OFF'}...</span>`;
  }

  try {
    const res = await fetch(enable ? '/api/bms_on' : '/api/bms_off', { method: 'POST' });
    if (res.ok) {
      setTimeout(() => { refreshData(); }, 700);
    }
  } catch(e) {
    if (statusSub) statusSub.innerHTML = `<span style="color:#dc2626;">Network error</span>`;
  } finally {
    if (btnOn) btnOn.disabled = false;
    if (btnOff) btnOff.disabled = false;
  }
}

// -------------------------------------------------------------
// JK-BMS Setup Modal & Bluetooth Scanner Functions
// -------------------------------------------------------------
let discoveredBmsDevices = [];

async function openBmsModal() {
  document.getElementById('bmsModal').classList.add('active');
  document.getElementById('bmsModalMsg').textContent = '';
  try {
    const res = await fetch('/api/bms_telemetry');
    if (res.ok) {
      const cfg = await res.json();
      document.getElementById('bmsEnabled').checked = !!cfg.enabled;
      document.getElementById('bmsDeviceMac').value = cfg.mac || '';
      document.getElementById('bmsDeviceName').value = cfg.name || '';
      document.getElementById('bmsPin').value = cfg.pin || '1234';
      if (cfg.connected) {
        document.getElementById('bmsScanStatus').textContent = `🟢 Currently connected to ${cfg.name || cfg.mac} (${cfg.rssi} dBm)`;
      } else {
        document.getElementById('bmsScanStatus').textContent = `Status: ${cfg.status || 'Disconnected'}`;
      }

      // Display live BLE Diagnostic status
      const diag = document.getElementById('bmsDiagInfo');
      if (diag && (cfg.connected || cfg.enabled)) {
        diag.style.display = 'block';
        let logsHtml = '';
        if (cfg.logs && cfg.logs.length) {
          logsHtml = `<div style="margin-top:6px; font-family:monospace; font-size:0.62rem; background:#1e293b; color:#38bdf8; padding:5px 8px; border-radius:6px; max-height:95px; overflow-y:auto; line-height:1.4;">` +
                     cfg.logs.map(l => escapeHtml(l)).join('<br>') + `</div>`;
        }
        diag.innerHTML = `<b>Live BLE Diagnostics:</b><br>` +
                         `• Status: <b>${escapeHtml(cfg.status || 'Unknown')}</b> | Write: <code>0x${(cfg.write_hdl || 0).toString(16).toUpperCase()}</code> | Notify: <code>0x${(cfg.notify_hdl || 0).toString(16).toUpperCase()}</code><br>` +
                         `• Packets Rx: <b>${cfg.rx_packets || 0}</b> (${cfg.rx_bytes || 0} B) | Tx: <b>${cfg.tx_cmds || 0}</b> | Last Hex: <code>${escapeHtml(cfg.last_hex || 'None')}</code>` +
                         logsHtml;
      } else if (diag) {
        diag.style.display = 'none';
      }
    }
  } catch(e) {}
}

function closeBmsModal() {
  document.getElementById('bmsModal').classList.remove('active');
  document.getElementById('bmsModalMsg').textContent = '';
}

async function scanBmsDevices() {
  const btn = document.getElementById('btnBmsScan');
  const statusEl = document.getElementById('bmsScanStatus');
  const sel = document.getElementById('bmsDeviceSelect');
  btn.disabled = true;
  btn.textContent = '⏳ Scanning...';
  statusEl.textContent = '🔍 Scanning nearby BLE devices (4s)...';

  try {
    const res = await fetch('/api/bms_scan');
    const list = await res.json();
    discoveredBmsDevices = Array.isArray(list) ? list : [];
    sel.innerHTML = '<option value="" disabled selected>-- Select Discovered BMS --</option>';

    if (!discoveredBmsDevices || discoveredBmsDevices.length === 0) {
      statusEl.textContent = 'No Bluetooth devices detected. Ensure JK-BMS is powered on and nearby.';
    } else {
      statusEl.textContent = `Found ${discoveredBmsDevices.length} Bluetooth devices:`;
      let autoSelected = false;
      discoveredBmsDevices.forEach(d => {
        const opt = document.createElement('option');
        opt.value = d.mac;
        opt.textContent = `${d.is_jk ? '⭐ [JK-BMS] ' : ''}${d.name} (${d.mac}) [${d.rssi} dBm]`;
        sel.appendChild(opt);

        // Auto select first JK device found if fields are empty
        if (d.is_jk && !autoSelected && !document.getElementById('bmsDeviceMac').value) {
          sel.value = d.mac;
          document.getElementById('bmsDeviceMac').value = d.mac;
          document.getElementById('bmsDeviceName').value = d.name;
          autoSelected = true;
        }
      });
    }
  } catch(e) {
    statusEl.textContent = 'Scan error. Please try again or type MAC address manually.';
  } finally {
    btn.disabled = false;
    btn.textContent = '🔍 Search Devices';
  }
}

function onBmsDeviceSelected() {
  const sel = document.getElementById('bmsDeviceSelect');
  const mac = sel.value;
  if (!mac) return;
  const dev = discoveredBmsDevices.find(d => d.mac === mac);
  if (dev) {
    document.getElementById('bmsDeviceMac').value = dev.mac;
    document.getElementById('bmsDeviceName').value = dev.name;
  } else {
    document.getElementById('bmsDeviceMac').value = mac;
  }
}

async function saveBmsConfig() {
  const enabled = document.getElementById('bmsEnabled').checked;
  const mac = document.getElementById('bmsDeviceMac').value.trim();
  const name = document.getElementById('bmsDeviceName').value.trim();
  const pin = document.getElementById('bmsPin').value.trim();
  const msgEl = document.getElementById('bmsModalMsg');

  if (enabled && !mac) {
    msgEl.style.color = '#ef4444';
    msgEl.textContent = 'Please select or enter the Bluetooth MAC address of your JK-BMS.';
    return;
  }

  msgEl.style.color = '#2563eb';
  msgEl.textContent = 'Saving configuration & connecting to JK-BMS...';

  try {
    const fd = new FormData();
    fd.append('enabled', enabled ? '1' : '0');
    fd.append('mac', mac);
    fd.append('name', name);
    fd.append('pin', pin);

    const res = await fetch('/api/bms_config', { method: 'POST', body: fd });
    if (res.ok) {
      msgEl.style.color = '#059669';
      msgEl.textContent = '✅ JK-BMS settings saved! ESP32 is connecting...';
      setTimeout(() => { closeBmsModal(); refreshData(); }, 1200);
    } else {
      msgEl.style.color = '#ef4444';
      msgEl.textContent = 'Failed to save settings.';
    }
  } catch(e) {
    msgEl.style.color = '#ef4444';
    msgEl.textContent = 'Network error saving BMS settings.';
  }
}

async function setBaud(b) {
  try {
    await fetch('/api/set_baud?baud=' + b, { method: 'POST' });
    document.getElementById('diagBaudBadge').textContent = b + ' bps';
    refreshData();
  } catch(e) {}
}

async function swapPins() {
  try {
    const res = await fetch('/api/swap_pins', { method: 'POST' });
    const txt = await res.text();
    alert('UART Pins: ' + txt);
    refreshData();
  } catch(e) {}
}

async function sendManualCmd(cmd) {
  try {
    document.getElementById('diagLastTx').textContent = cmd + ' (sending...)';
    await fetch('/api/send_cmd?cmd=' + encodeURIComponent(cmd), { method: 'POST' });
    refreshData();
  } catch(e) {}
}

function updateTbStatus(enabled, status) {
  const pill = document.getElementById('tbStatusPill');
  if (!pill) return;
  if (!enabled) {
    pill.style.background = '#e2e8f0';
    pill.style.color = '#475569';
    pill.textContent = 'Disabled';
  } else {
    pill.style.background = '#dcfce7';
    pill.style.color = '#15803d';
    pill.textContent = status || 'Active';
  }
}

async function loadTbConfig() {
  try {
    const res = await fetch('/api/tb_config');
    if (res.ok) {
      const d = await res.json();
      document.getElementById('tbEnabled').checked = !!d.enabled;
      if (d.server) document.getElementById('tbServer').value = d.server;
      if (d.token) document.getElementById('tbToken').value = d.token;
      if (d.interval) document.getElementById('tbInterval').value = String(d.interval);
      updateTbStatus(d.enabled, d.last_status);
    }
  } catch(e) {}
}

async function saveTbConfig() {
  const enabled = document.getElementById('tbEnabled').checked;
  const server = document.getElementById('tbServer').value.trim();
  const token = document.getElementById('tbToken').value.trim();
  const interval = document.getElementById('tbInterval').value;
  
  const msg = document.getElementById('modalMsg');
  msg.style.color = '#2563eb';
  msg.textContent = 'Saving ThingsBoard configuration...';

  try {
    const res = await fetch('/api/tb_config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `enabled=${enabled}&server=${encodeURIComponent(server)}&token=${encodeURIComponent(token)}&interval=${interval}`
    });
    if (res.ok) {
      msg.style.color = '#10b981';
      msg.textContent = '✅ ThingsBoard settings saved!';
      updateTbStatus(enabled, enabled ? 'Enabled' : 'Disabled');
    } else {
      msg.style.color = '#ef4444';
      msg.textContent = 'Failed to save ThingsBoard settings.';
    }
  } catch(e) {
    msg.style.color = '#ef4444';
    msg.textContent = 'Network error saving settings.';
  }
}

function updateBlynkStatus(enabled, status) {
  const pill = document.getElementById('blynkStatusPill');
  if (!pill) return;
  if (!enabled) {
    pill.style.background = '#e2e8f0';
    pill.style.color = '#475569';
    pill.textContent = 'Disabled';
  } else {
    pill.style.background = '#dcfce7';
    pill.style.color = '#15803d';
    pill.textContent = status || 'Active';
  }
}

async function loadBlynkConfig() {
  try {
    const res = await fetch('/api/blynk_config');
    if (res.ok) {
      const d = await res.json();
      document.getElementById('blynkEnabled').checked = !!d.enabled;
      if (d.server) document.getElementById('blynkServer').value = d.server;
      if (d.token) document.getElementById('blynkToken').value = d.token;
      if (d.interval) document.getElementById('blynkInterval').value = String(d.interval);
      updateBlynkStatus(d.enabled, d.last_status);
    }
  } catch(e) {}
}

async function saveBlynkConfig() {
  const enabled = document.getElementById('blynkEnabled').checked;
  const server = document.getElementById('blynkServer').value.trim();
  const token = document.getElementById('blynkToken').value.trim();
  const interval = document.getElementById('blynkInterval').value;
  
  const msg = document.getElementById('modalMsg');
  msg.style.color = '#059669';
  msg.textContent = 'Saving Blynk configuration...';

  try {
    const res = await fetch('/api/blynk_config', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: `enabled=${enabled}&server=${encodeURIComponent(server)}&token=${encodeURIComponent(token)}&interval=${interval}`
    });
    if (res.ok) {
      msg.style.color = '#10b981';
      msg.textContent = '✅ Blynk settings saved!';
      updateBlynkStatus(enabled, enabled ? 'Enabled' : 'Disabled');
    } else {
      msg.style.color = '#ef4444';
      msg.textContent = 'Failed to save Blynk settings.';
    }
  } catch(e) {
    msg.style.color = '#ef4444';
    msg.textContent = 'Network error saving Blynk settings.';
  }
}

function openWifiModal() {
  document.getElementById('wifiModal').classList.add('active');
  scanWifiNetworks();
  loadTbConfig();
  loadBlynkConfig();
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

// -------------------------------------------------------------
// Inverter Mode Macros Engine
// -------------------------------------------------------------
let userMacros = [];

async function loadMacros() {
  try {
    const res = await fetch('/api/macros');
    if (res.ok) {
      userMacros = await res.json();
      renderMacros();
    }
  } catch(e) {
    document.getElementById('macrosContainer').innerHTML = '<div style="text-align:center; padding:10px; color:#ef4444; font-size:0.75rem;">Error loading macros.</div>';
  }
}

function renderMacros() {
  const container = document.getElementById('macrosContainer');
  if (!userMacros || userMacros.length === 0) {
    container.innerHTML = '<div style="text-align:center; padding:14px; color:#64748b; font-size:0.75rem;">No macros created yet.<br>Click <b>+ Create Macro</b> above to create your first 1-click preset.</div>';
    return;
  }

  let html = '';
  userMacros.forEach(m => {
    html += `
      <div class="macro-item">
        <div class="macro-info">
          <div class="macro-name">
            <span>${escapeHtml(m.name)}</span>
          </div>
          <div class="macro-desc">${escapeHtml(m.desc || 'Custom Inverter Settings Preset')}</div>
        </div>
        <div class="macro-actions">
          <button class="btn-apply-macro" id="btnApply_${m.id}" onclick="applyMacro('${m.id}')">Apply</button>
          <button class="btn-icon-macro" title="Edit Macro" onclick="editMacro('${m.id}')">✏️</button>
          <button class="btn-icon-macro" title="Delete Macro" onclick="deleteMacro('${m.id}')">🗑️</button>
        </div>
      </div>
    `;
  });
  container.innerHTML = html;
}

function escapeHtml(str) {
  if (!str) return '';
  return str.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;").replace(/'/g, "&#039;");
}

function openCreateMacroModal() {
  document.getElementById('macroModalTitle').textContent = '⚡ Create Inverter Macro';
  document.getElementById('editMacroId').value = '';
  document.getElementById('macroInputName').value = '';
  document.getElementById('macroOutPriority').value = '';
  document.getElementById('macroChgPriority').value = '';
  document.getElementById('macroMaxChgCurr').value = '';
  document.getElementById('macroMaxUtilCurr').value = '';
  document.getElementById('macroOverloadBypass').value = '';
  document.getElementById('macroBackGridVolt').value = '';
  document.getElementById('macroBackBatVolt').value = '';
  document.getElementById('macroModalMsg').textContent = '';
  document.getElementById('macroModal').classList.add('active');
}

function closeMacroModal() {
  document.getElementById('macroModal').classList.remove('active');
  document.getElementById('macroModalMsg').textContent = '';
}

function editMacro(id) {
  const m = userMacros.find(x => x.id === id);
  if (!m) return;

  document.getElementById('macroModalTitle').textContent = '✏️ Edit Inverter Macro';
  document.getElementById('editMacroId').value = m.id;
  document.getElementById('macroInputName').value = m.name || '';
  
  // Reset fields first
  document.getElementById('macroOutPriority').value = '';
  document.getElementById('macroChgPriority').value = '';
  document.getElementById('macroMaxChgCurr').value = '';
  document.getElementById('macroMaxUtilCurr').value = '';
  document.getElementById('macroOverloadBypass').value = '';
  document.getElementById('macroBackGridVolt').value = '';
  document.getElementById('macroBackBatVolt').value = '';

  // Parse existing commands
  if (Array.isArray(m.cmds)) {
    m.cmds.forEach(cmd => {
      if (cmd.startsWith('POP')) document.getElementById('macroOutPriority').value = cmd;
      else if (cmd.startsWith('PCP')) document.getElementById('macroChgPriority').value = cmd;
      else if (cmd.startsWith('MNCHGC') || cmd.startsWith('MCHGC')) document.getElementById('macroMaxChgCurr').value = cmd.replace('MCHGC', 'MNCHGC');
      else if (cmd.startsWith('MUCHGC')) document.getElementById('macroMaxUtilCurr').value = cmd;
      else if (cmd === 'PEb' || cmd === 'PDb') document.getElementById('macroOverloadBypass').value = cmd;
      else if (cmd.startsWith('PBCV')) document.getElementById('macroBackGridVolt').value = parseFloat(cmd.substring(4));
      else if (cmd.startsWith('PBDV')) document.getElementById('macroBackBatVolt').value = parseFloat(cmd.substring(4));
    });
  }

  document.getElementById('macroModalMsg').textContent = '';
  document.getElementById('macroModal').classList.add('active');
}

async function saveMacroPreset() {
  const name = document.getElementById('macroInputName').value.trim();
  const msgEl = document.getElementById('macroModalMsg');

  if (!name) {
    msgEl.style.color = '#ef4444';
    msgEl.textContent = 'Please enter a name for this macro button.';
    return;
  }

  const cmds = [];
  const descParts = [];

  // 1. Output Priority (Full Words!)
  const outPri = document.getElementById('macroOutPriority').value;
  if (outPri) {
    cmds.push(outPri);
    if (outPri === 'POP02') descParts.push('Solar First, Battery Second, Utility Grid Last (SBU Priority)');
    else if (outPri === 'POP01') descParts.push('Solar First, Utility Grid Blending (SUB Mode)');
    else if (outPri === 'POP00') descParts.push('Utility Grid First (USB Mode)');
  }

  // 2. Charger Priority (Full Words!)
  const chgPri = document.getElementById('macroChgPriority').value;
  if (chgPri) {
    cmds.push(chgPri);
    if (chgPri === 'PCP02') descParts.push('Only Solar Charging (OSO - 100% Free Solar)');
    else if (chgPri === 'PCP01') descParts.push('Solar & Utility Simultaneous Charging (SNU)');
    else if (chgPri === 'PCP03') descParts.push('Solar Energy First (CSO)');
    else if (chgPri === 'PCP00') descParts.push('Utility Grid First Charging');
  }

  // 3. Max Charge Current (Full Words!)
  const maxChg = document.getElementById('macroMaxChgCurr').value;
  if (maxChg) {
    cmds.push(maxChg);
    const amps = parseInt(maxChg.replace('MNCHGC', '').replace('MCHGC', ''), 10);
    descParts.push(`Max Total Charge: ${amps} Amps`);
  }

  // 4. Max Utility Current (Full Words!)
  const maxUtil = document.getElementById('macroMaxUtilCurr').value;
  if (maxUtil) {
    cmds.push(maxUtil);
    const amps = parseInt(maxUtil.substring(6), 10);
    descParts.push(`Max Utility Charge: ${amps} Amps`);
  }

  // 5. Overload Bypass (Full Words!)
  const ovr = document.getElementById('macroOverloadBypass').value;
  if (ovr) {
    cmds.push(ovr);
    if (ovr === 'PEb') descParts.push('Overload Bypass to Grid Enabled');
    else if (ovr === 'PDb') descParts.push('Overload Bypass Disabled');
  }

  // 6. Custom Battery Voltages (Optional)
  const backGridEl = document.getElementById('macroBackGridVolt');
  const backGridV = backGridEl ? parseFloat(backGridEl.value) : NaN;
  if (!isNaN(backGridV) && backGridV >= 42.0 && backGridV <= 51.0) {
    cmds.push('PBCV' + backGridV.toFixed(1));
    descParts.push(`Back to Grid: ${backGridV.toFixed(1)}V`);
  }

  const backBatEl = document.getElementById('macroBackBatVolt');
  const backBatV = backBatEl ? parseFloat(backBatEl.value) : NaN;
  if (!isNaN(backBatV) && backBatV >= 48.0 && backBatV <= 58.0) {
    cmds.push('PBDV' + backBatV.toFixed(1));
    descParts.push(`Back to Battery: ${backBatV.toFixed(1)}V`);
  }

  if (cmds.length === 0) {
    msgEl.style.color = '#ef4444';
    msgEl.textContent = 'Please select at least one inverter setting for this macro.';
    return;
  }

  msgEl.style.color = '#2563eb';
  msgEl.textContent = 'Saving macro preset to ESP32...';

  const editId = document.getElementById('editMacroId').value;
  const macroObj = {
    id: editId || ('m_' + Date.now()),
    name: name,
    desc: descParts.join(' • '),
    cmds: cmds
  };

  let updatedList = [...userMacros];
  if (editId) {
    const idx = updatedList.findIndex(x => x.id === editId);
    if (idx >= 0) updatedList[idx] = macroObj;
    else updatedList.push(macroObj);
  } else {
    updatedList.push(macroObj);
  }

  try {
    const res = await fetch('/api/save_macros', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(updatedList)
    });
    if (res.ok) {
      userMacros = updatedList;
      renderMacros();
      closeMacroModal();
      showMacroBanner(`✅ Macro button "${name}" created and saved!`, 'success');
    } else {
      msgEl.style.color = '#ef4444';
      msgEl.textContent = 'Failed to save macro to ESP32.';
    }
  } catch(e) {
    msgEl.style.color = '#ef4444';
    msgEl.textContent = 'Network error saving macro.';
  }
}

async function deleteMacro(id) {
  const m = userMacros.find(x => x.id === id);
  if (!m) return;
  if (!confirm(`Delete macro preset "${m.name}"?`)) return;

  const updatedList = userMacros.filter(x => x.id !== id);
  try {
    const res = await fetch('/api/save_macros', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(updatedList)
    });
    if (res.ok) {
      userMacros = updatedList;
      renderMacros();
      showMacroBanner(`🗑️ Deleted macro "${m.name}".`, 'success');
    }
  } catch(e) {
    alert('Failed to delete macro.');
  }
}

async function applyMacro(id) {
  const m = userMacros.find(x => x.id === id);
  if (!m || !m.cmds || m.cmds.length === 0) return;

  const btn = document.getElementById(`btnApply_${id}`);
  if (btn) {
    btn.disabled = true;
    btn.textContent = 'Applying...';
  }

  isMacroApplying = true;
  showMacroBanner(`⏳ Applying "${escapeHtml(m.name)}" to Inverter via RS-232 (${m.cmds.length} settings)...`, 'running');

  try {
    const res = await fetch('/api/run_macro', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'cmds=' + encodeURIComponent(m.cmds.join(','))
    });

    if (res.ok) {
      const data = await res.json();
      if (data.all_ack) {
        let details = (data.results || []).map(r => `<b>${escapeHtml(r.cmd)}</b>: ACK`).join(', ');
        showMacroBanner(`✅ <b>"${escapeHtml(m.name)}" applied successfully!</b><br><span style="font-size:0.7rem; color:#065f46;">All settings confirmed by Inverter (${details})</span>`, 'success');
      } else {
        let errHtml = `<div>⚠️ <b>"${escapeHtml(m.name)}" executed with responses:</b></div><div style="margin-top:5px; font-size:0.72rem; line-height:1.5;">`;
        if (data.results && data.results.length > 0) {
          data.results.forEach(r => {
            if (r.ack) {
              errHtml += `<div>• <b>${escapeHtml(r.cmd)}</b>: <span style="color:#059669; font-weight:700;">✅ Confirmed (ACK)</span></div>`;
            } else {
              errHtml += `<div>• <b>${escapeHtml(r.cmd)}</b>: <span style="color:#dc2626; font-weight:700;">❌ Rejected / No ACK</span> <span style="color:#475569; font-size:0.68rem;">(Inverter reply: <code>${escapeHtml(r.resp || 'Timeout')}</code>)</span></div>`;
            }
          });
        }
        errHtml += `</div><div style="font-size:0.68rem; color:#64748b; margin-top:4px;">Check if the inverter is in a mode that permits changing these specific parameters.</div>`;
        showMacroBanner(errHtml, 'error');
      }
    } else {
      showMacroBanner(`❌ <b>Error:</b> ESP32 could not process the macro request.`, 'error');
    }
  } catch(e) {
    showMacroBanner(`❌ <b>Network error:</b> Could not communicate with ESP32.`, 'error');
  } finally {
    isMacroApplying = false;
    if (btn) {
      btn.disabled = false;
      btn.textContent = 'Apply';
    }
    setTimeout(refreshData, 600);
  }
}

let macroBannerTimer = null;
function showMacroBanner(msgHtml, type) {
  const banner = document.getElementById('macroStatusBanner');
  if (!banner) return;
  banner.className = `macro-status-banner ${type}`;
  banner.innerHTML = msgHtml;
  banner.style.display = 'block';

  if (macroBannerTimer) clearTimeout(macroBannerTimer);
  if (type === 'success') {
    macroBannerTimer = setTimeout(() => {
      banner.style.display = 'none';
    }, 8000);
  }
}

// ==================== 24h Solar & Weather Forecast ====================
let currentWeatherCity = {
  name: "Colombo",
  lat: 6.9271,
  lon: 79.8612
};

function initWeatherCity() {
  try {
    const saved = localStorage.getItem('next_inv_weather_city');
    if (saved) {
      const parsed = JSON.parse(saved);
      if (parsed && parsed.name && parsed.lat && parsed.lon) {
        currentWeatherCity = parsed;
      }
    }
  } catch(e) {}
  updateCityHeaderDisplay();
}

function updateCityHeaderDisplay() {
  const el = document.getElementById('weatherCityName');
  if (el) el.textContent = (currentWeatherCity.name || 'COLOMBO').toUpperCase();
}

// Generate TV broadcast style vector weather illustrations
function getWeatherSvgIcon(code, isDay) {
  if (isDay) {
    if (code === 0) {
      // Clear Brilliant Sun with Radiant Golden Rays
      return `<svg width="44" height="44" viewBox="0 0 48 48">
        <defs>
          <radialGradient id="sunG" cx="50%" cy="50%" r="50%">
            <stop offset="0%" stop-color="#fffbeb"/>
            <stop offset="35%" stop-color="#fbbf24"/>
            <stop offset="100%" stop-color="#f59e0b"/>
          </radialGradient>
        </defs>
        <g stroke="#f59e0b" stroke-width="2.5" stroke-linecap="round">
          <line x1="24" y1="3" x2="24" y2="8"/><line x1="24" y1="40" x2="24" y2="45"/>
          <line x1="3" y1="24" x2="8" y2="24"/><line x1="40" y1="24" x2="45" y2="24"/>
          <line x1="9" y1="9" x2="13" y2="13"/><line x1="35" y1="35" x2="39" y2="39"/>
          <line x1="9" y1="39" x2="13" y2="35"/><line x1="35" y1="13" x2="39" y2="9"/>
        </g>
        <circle cx="24" cy="24" r="11" fill="url(#sunG)"/>
      </svg>`;
    } else if (code === 1 || code === 2) {
      // Radiant Sun with Floating Soft Cloud in Front (matching TV screenshot)
      return `<svg width="44" height="44" viewBox="0 0 48 48">
        <circle cx="28" cy="18" r="9" fill="#f59e0b"/>
        <g stroke="#f59e0b" stroke-width="2" stroke-linecap="round" opacity="0.8">
          <line x1="28" y1="4" x2="28" y2="7"/><line x1="42" y1="18" x2="39" y2="18"/>
          <line x1="37" y1="9" x2="34" y2="12"/><line x1="37" y1="27" x2="34" y2="24"/>
        </g>
        <path d="M11 36 A6 6 0 0 1 18 24 A9 9 0 0 1 34 26 A6 6 0 0 1 38 36 Z" fill="#ffffff" filter="drop-shadow(0 2px 3px rgba(15,23,42,0.18))"/>
      </svg>`;
    } else if (code === 3 || code === 45 || code === 48) {
      // Overcast / Fog
      return `<svg width="44" height="44" viewBox="0 0 48 48">
        <path d="M14 28 A7 7 0 0 1 23 16 A11 11 0 0 1 40 22 A8 8 0 0 1 42 34 Z" fill="#cbd5e1" opacity="0.75"/>
        <path d="M8 36 A7 7 0 0 1 16 24 A10 10 0 0 1 33 27 A7 7 0 0 1 37 36 Z" fill="#ffffff" filter="drop-shadow(0 2px 3px rgba(15,23,42,0.15))"/>
      </svg>`;
    } else if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) {
      // Rain / Showers
      return `<svg width="44" height="44" viewBox="0 0 48 48">
        <path d="M10 27 A7 7 0 0 1 18 16 A10 10 0 0 1 35 19 A7 7 0 0 1 39 27 Z" fill="#94a3b8"/>
        <g stroke="#38bdf8" stroke-width="2.5" stroke-linecap="round">
          <line x1="16" y1="32" x2="14" y2="38"/>
          <line x1="25" y1="32" x2="23" y2="38"/>
          <line x1="33" y1="32" x2="31" y2="38"/>
        </g>
      </svg>`;
    } else if (code >= 95) {
      // Thunderstorm
      return `<svg width="44" height="44" viewBox="0 0 48 48">
        <path d="M10 25 A7 7 0 0 1 18 13 A10 10 0 0 1 35 16 A7 7 0 0 1 39 25 Z" fill="#475569"/>
        <polygon points="26,24 20,33 26,33 22,42 30,30 24,30" fill="#facc15" stroke="#ca8a04" stroke-width="1"/>
      </svg>`;
    }
  } else {
    // Night
    if (code === 0 || code === 1) {
      // Clear Moon with Stars
      return `<svg width="44" height="44" viewBox="0 0 48 48">
        <path d="M28 8 A14 14 0 1 0 38 32 A13 13 0 0 1 28 8 Z" fill="#fbbf24"/>
        <circle cx="12" cy="14" r="1.5" fill="#ffffff" opacity="0.9"/>
        <circle cx="36" cy="12" r="1" fill="#ffffff" opacity="0.8"/>
      </svg>`;
    } else {
      // Night Cloud
      return `<svg width="44" height="44" viewBox="0 0 48 48">
        <path d="M26 10 A11 11 0 1 0 34 28 A10 10 0 0 1 26 10 Z" fill="#fbbf24" opacity="0.85"/>
        <path d="M10 36 A6 6 0 0 1 17 25 A9 9 0 0 1 32 27 A6 6 0 0 1 36 36 Z" fill="#94a3b8" opacity="0.9"/>
      </svg>`;
    }
  }
  return `<svg width="44" height="44" viewBox="0 0 48 48"><circle cx="24" cy="24" r="10" fill="#f59e0b"/></svg>`;
}

function openCityModal() {
  document.getElementById('cityModal').classList.add('active');
  const searchInp = document.getElementById('citySearchInput');
  if (searchInp) {
    searchInp.value = '';
    searchInp.focus();
  }
  document.getElementById('citySearchResults').innerHTML = '';
}

function closeCityModal() {
  document.getElementById('cityModal').classList.remove('active');
}

function selectCity(name, lat, lon) {
  currentWeatherCity = { name, lat, lon };
  try {
    localStorage.setItem('next_inv_weather_city', JSON.stringify(currentWeatherCity));
  } catch(e) {}
  updateCityHeaderDisplay();
  closeCityModal();
  fetchWeatherForecast(true);
}

async function searchCustomCity() {
  const q = document.getElementById('citySearchInput').value.trim();
  const resContainer = document.getElementById('citySearchResults');
  if (!q) return;
  resContainer.innerHTML = '<div style="font-size:0.75rem; color:#64748b; padding:8px 0;">🔍 Searching...</div>';
  try {
    const res = await fetch(`https://geocoding-api.open-meteo.com/v1/search?name=${encodeURIComponent(q)}&count=6&language=en&format=json`);
    if (!res.ok) throw new Error("Search failed");
    const data = await res.json();
    if (!data.results || data.results.length === 0) {
      resContainer.innerHTML = '<div style="font-size:0.75rem; color:#ef4444; padding:8px 0;">No locations found. Try another city name.</div>';
      return;
    }
    let html = '';
    data.results.forEach(item => {
      const label = `${item.name}${item.admin1 ? ', ' + item.admin1 : ''} (${item.country || ''})`;
      html += `<div class="city-search-item" onclick="selectCity('${escapeHtml(item.name + (item.country ? ' (' + item.country + ')' : ''))}', ${item.latitude}, ${item.longitude})">
        <div>
          <b>${escapeHtml(item.name)}</b>
          <span style="font-size:0.68rem; color:#64748b; margin-left:4px;">${escapeHtml(item.admin1 || '')} ${escapeHtml(item.country || '')}</span>
        </div>
        <span style="font-size:0.68rem; color:#2563eb; font-weight:700;">Select →</span>
      </div>`;
    });
    resContainer.innerHTML = html;
  } catch(e) {
    resContainer.innerHTML = '<div style="font-size:0.75rem; color:#ef4444; padding:8px 0;">Error connecting to search service.</div>';
  }
}

// Map WMO Weather Codes to Emoji and Descriptions
function getWeatherIconAndDesc(code, isDay) {
  switch (code) {
    case 0:
      return isDay ? { icon: "☀️", desc: "Clear Sun" } : { icon: "🌙", desc: "Clear Night" };
    case 1:
      return isDay ? { icon: "🌤️", desc: "Mainly Clear" } : { icon: "🌤️", desc: "Mainly Clear" };
    case 2:
      return { icon: "⛅", desc: "Partly Cloudy" };
    case 3:
      return { icon: "☁️", desc: "Overcast" };
    case 45:
    case 48:
      return { icon: "🌫️", desc: "Fog" };
    case 51:
    case 53:
    case 55:
      return { icon: "🌦️", desc: "Light Drizzle" };
    case 61:
    case 63:
    case 65:
      return { icon: "🌧️", desc: "Rain" };
    case 80:
    case 81:
    case 82:
      return { icon: "🌧️", desc: "Rain Showers" };
    case 95:
    case 96:
    case 99:
      return { icon: "⛈️", desc: "Thunderstorm" };
    default:
      return isDay ? { icon: "🌤️", desc: "Fair" } : { icon: "🌙", desc: "Fair" };
  }
}

let lastWeatherFetchTime = 0;
let cachedWeatherData = null;

async function fetchWeatherForecast(force = false) {
  const nowMs = Date.now();
  if (!force && cachedWeatherData && (nowMs - lastWeatherFetchTime < 30 * 60 * 1000)) {
    renderWeather(cachedWeatherData);
    return;
  }

  const updatedTimeEl = document.getElementById('weatherUpdatedTime');
  if (updatedTimeEl) updatedTimeEl.textContent = '• Updating...';

  try {
    const url = `https://api.open-meteo.com/v1/forecast?latitude=${currentWeatherCity.lat}&longitude=${currentWeatherCity.lon}&hourly=temperature_2m,precipitation_probability,weather_code,cloud_cover,is_day,direct_normal_irradiance&forecast_days=2&timezone=auto`;
    const res = await fetch(url);
    if (!res.ok) throw new Error("Forecast API failed");
    const data = await res.json();
    cachedWeatherData = data;
    lastWeatherFetchTime = nowMs;
    renderWeather(data);
    if (updatedTimeEl) updatedTimeEl.textContent = `• Updated ${new Date().toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'})}`;
  } catch(e) {
    if (updatedTimeEl) updatedTimeEl.textContent = '• Offline / Failed';
    const advisoryDesc = document.getElementById('advisoryDesc');
    if (advisoryDesc && !cachedWeatherData) {
      advisoryDesc.textContent = 'Unable to fetch weather forecast. Connect to the internet to get solar predictions.';
    }
  }
}

function renderWeather(data) {
  if (!data || !data.hourly || !data.hourly.time) return;

  const times = data.hourly.time;
  const temps = data.hourly.temperature_2m;
  const rains = data.hourly.precipitation_probability;
  const codes = data.hourly.weather_code;
  const clouds = data.hourly.cloud_cover;
  const isDays = data.hourly.is_day;
  const directRad = data.hourly.direct_normal_irradiance || [];

  const now = new Date();
  let startIdx = -1;
  for (let i = 0; i < times.length; i++) {
    const t = new Date(times[i]);
    if (t <= now && (i === times.length - 1 || new Date(times[i + 1]) > now)) {
      startIdx = i;
      break;
    }
  }
  if (startIdx === -1) startIdx = 0;

  const count = Math.min(24, times.length - startIdx);
  const next24 = [];
  for (let i = 0; i < count; i++) {
    const idx = startIdx + i;
    next24.push({
      time: new Date(times[idx]),
      temp: Math.round(temps[idx]),
      rain: rains[idx] !== null && rains[idx] !== undefined ? rains[idx] : 0,
      code: codes[idx] !== null && codes[idx] !== undefined ? codes[idx] : 0,
      cloud: clouds[idx] !== null && clouds[idx] !== undefined ? clouds[idx] : 0,
      isDay: isDays[idx] === 1,
      irradiance: directRad[idx] ? Math.round(directRad[idx]) : 0,
      isNow: (i === 0)
    });
  }

  // --- Calculate Daylight Solar Advisory ---
  const daylightHours = next24.filter(h => h.isDay);
  
  let avgCloud = 0;
  let maxRain = 0;
  let peakIrradiance = 0;

  if (daylightHours.length > 0) {
    avgCloud = Math.round(daylightHours.reduce((acc, h) => acc + h.cloud, 0) / daylightHours.length);
    maxRain = Math.max(...daylightHours.map(h => h.rain));
    peakIrradiance = Math.max(...daylightHours.map(h => h.irradiance));
  } else {
    avgCloud = Math.round(next24.reduce((acc, h) => acc + h.cloud, 0) / next24.length);
    maxRain = Math.max(...next24.map(h => h.rain));
  }

  // Metric Summary Boxes
  const sumSolar = document.getElementById('summarySolar');
  if (sumSolar) {
    if (peakIrradiance > 600 || avgCloud < 40) {
      sumSolar.innerHTML = `<span style="color:#059669;">☀️ Strong</span>`;
    } else if (peakIrradiance > 300 || avgCloud < 70) {
      sumSolar.innerHTML = `<span style="color:#d97706;">⛅ Moderate</span>`;
    } else {
      sumSolar.innerHTML = `<span style="color:#dc2626;">☁️ Weak</span>`;
    }
  }

  const sumRain = document.getElementById('summaryRain');
  if (sumRain) {
    sumRain.innerHTML = `${maxRain}%`;
    sumRain.style.color = (maxRain >= 50) ? '#2563eb' : '#1e293b';
  }

  const sumCloud = document.getElementById('summaryCloud');
  if (sumCloud) {
    sumCloud.innerHTML = `${avgCloud}%`;
    sumCloud.style.color = (avgCloud >= 70) ? '#dc2626' : (avgCloud < 40 ? '#059669' : '#1e293b');
  }

  // Battery Decision & Strategy Advisory
  const advBanner = document.getElementById('solarAdvisoryBanner');
  const advTitle = document.getElementById('advisoryTitle');
  const advDesc = document.getElementById('advisoryDesc');
  const advStats = document.getElementById('advisoryStats');

  if (advBanner && advTitle && advDesc && advStats) {
    if (avgCloud < 45 && maxRain < 40) {
      advBanner.className = 'solar-advisory sunny';
      advTitle.innerHTML = '☀️ <b>High Solar Expected: Bill Reduction Mode</b>';
      advDesc.innerHTML = '<b>Let battery discharge tonight to power loads!</b> Tomorrow brings strong sunshine to fully recharge your battery, maximizing CEB electricity bill savings.';
      advStats.innerHTML = `<span>☀️ Peak Solar: ~${peakIrradiance || '750+'} W/m²</span> <span>☁️ Avg Clouds: ${avgCloud}%</span> <span>💧 Max Rain: ${maxRain}%</span>`;
    } else if (avgCloud >= 70 || maxRain >= 60) {
      advBanner.className = 'solar-advisory rainy';
      advTitle.innerHTML = '⛈️ <b>Poor Solar Warning: Emergency Grid Charge</b>';
      advDesc.innerHTML = '<b>Charge battery to full from grid tonight!</b> Tomorrow has heavy clouds and rain. Preserve full battery reserve for blackouts and emergency backup.';
      advStats.innerHTML = `<span>⛈️ Rain Risk: ${maxRain}%</span> <span>☁️ Heavy Clouds: ${avgCloud}%</span> <span>⚡ Grid Charge Recommended</span>`;
    } else {
      advBanner.className = 'solar-advisory moderate';
      advTitle.innerHTML = '⛅ <b>Moderate Solar Expected: Balanced Mode</b>';
      advDesc.innerHTML = '<b>Maintain normal battery buffer tonight (~30-40%).</b> Tomorrow will have partly cloudy skies with intermittent solar generation.';
      advStats.innerHTML = `<span>⛅ Solar: Moderate</span> <span>☁️ Avg Clouds: ${avgCloud}%</span> <span>💧 Rain Risk: ${maxRain}%</span>`;
    }
  }

  // Render 24-Hour Timeline
  const timelineEl = document.getElementById('weatherTimeline');
  if (timelineEl) {
    let timelineHtml = '';
    next24.forEach(h => {
      const info = getWeatherIconAndDesc(h.code, h.isDay);
      const timeStr = h.isNow ? 'NOW' : h.time.toLocaleTimeString([], { hour: 'numeric', hour12: true }).toUpperCase();

      let solarSubText = '🌙 Night';
      if (h.isDay) {
        if (h.irradiance > 500 || h.cloud < 30) {
          solarSubText = `☀️ ${h.irradiance ? h.irradiance + ' W/m²' : 'Strong Sun'}`;
        } else if (h.irradiance > 200 || h.cloud < 65) {
          solarSubText = `⛅ ${h.irradiance ? h.irradiance + ' W/m²' : 'Moderate'}`;
        } else {
          solarSubText = `☁️ Low Solar`;
        }
      }

      timelineHtml += `
      <div class="weather-hour-col ${h.isNow ? 'now' : ''} ${!h.isDay ? 'night-col' : ''}">
        <div class="weather-col-header">${timeStr}</div>
        <div class="weather-col-body">
          <div class="weather-col-icon" title="${escapeHtml(info.desc)}">
            ${getWeatherSvgIcon(h.code, h.isDay)}
          </div>
          <div class="weather-col-temp">${h.temp}<span class="temp-deg">°</span></div>
          <div class="weather-col-sub">${solarSubText}</div>
        </div>
        <div class="weather-col-rain ${h.rain >= 40 ? 'rain-alert' : ''}">
          <span>${h.rain}%</span>
          <svg width="10" height="11" viewBox="0 0 10 12" fill="currentColor"><path d="M5 0C5 0 0 6 0 8.5C0 10.4 2.2 12 5 12C7.8 12 10 10.4 10 8.5C10 6 5 0 5 0Z"/></svg>
        </div>
      </div>`;
    });
    timelineEl.innerHTML = timelineHtml;
  }
}

// Light-dismiss modals on background click or Esc key
document.querySelectorAll('.modal-overlay').forEach(overlay => {
  overlay.addEventListener('click', (e) => {
    if (e.target === overlay) {
      overlay.classList.remove('active');
    }
  });
});
window.addEventListener('keydown', (e) => {
  if (e.key === 'Escape') {
    document.querySelectorAll('.modal-overlay.active').forEach(m => m.classList.remove('active'));
  }
});

// -------------------------------------------------------------
// Dashboard Card Alignment / Layout Switcher Controller
// -------------------------------------------------------------
function setDashboardLayout(mode) {
  const pw = document.getElementById('pageWrapper');
  if (!pw) return;

  document.body.classList.remove('layout-1col', 'layout-2col', 'layout-3col');
  pw.classList.remove('layout-1col', 'layout-2col', 'layout-3col');

  if (mode === '1col') {
    document.body.classList.add('layout-1col');
    pw.classList.add('layout-1col');
  } else if (mode === '2col') {
    document.body.classList.add('layout-2col');
    pw.classList.add('layout-2col');
  } else if (mode === '3col') {
    document.body.classList.add('layout-3col');
    pw.classList.add('layout-3col');
  }

  document.querySelectorAll('.btn-layout-opt').forEach(b => b.classList.remove('active'));
  const btn = document.getElementById('btnLayout' + mode.charAt(0).toUpperCase() + mode.slice(1));
  if (btn) btn.classList.add('active');

  try {
    localStorage.setItem('dash_card_layout', mode);
  } catch(e) {}
}

function initDashboardLayout() {
  try {
    const savedLayout = localStorage.getItem('dash_card_layout') || 'auto';
    setDashboardLayout(savedLayout);
  } catch(e) {
    setDashboardLayout('auto');
  }
}

initDashboardLayout();
setInterval(refreshData, 2000);
setInterval(() => fetchWeatherForecast(false), 30 * 60 * 1000); // 30 min weather refresh
refreshData();
loadMacros();
initWeatherCity();
fetchWeatherForecast();
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
