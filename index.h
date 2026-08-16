#ifndef INDEX_H
#define INDEX_H

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Smart Irrigation System</title>
  
  <!-- PWA & Android App Support -->
  <link rel="manifest" href="data:application/manifest+json;utf8,{%22name%22:%22Smart%20Irrigation%22,%22short_name%22:%22Irrigation%22,%22start_url%22:%22/%22,%22display%22:%22standalone%22,%22background_color%22:%22%23000000%22,%22theme_color%22:%22%23000000%22}">
  <meta name="mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="theme-color" content="#000000">
  
  <link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='.9em' font-size='90'>🌿</text></svg>">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/qrious/4.0.2/qrious.min.js"></script>
  <style>
    :root {
      --bg: #000000;
      --card: rgba(28, 28, 30, 0.65);
      --card-border: rgba(255, 255, 255, 0.12);
      --accent: #0a84ff;
      --text: #f5f5f7;
      --muted: #8e8e93;
      --ok: #30d158;
      --err: #ff453a;
      --warn: #ffd60a;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "SF Pro Display", "SF Pro Text", "Helvetica Neue", sans-serif; -webkit-font-smoothing: antialiased; }
    body { background: var(--bg); color: var(--text); padding-bottom: 40px; overflow-x: hidden; }
    
    #loginOverlay {
      position: fixed; inset: 0; background: rgba(0, 0, 0, 0.85); backdrop-filter: blur(25px); -webkit-backdrop-filter: blur(25px);
      display: flex; justify-content: center; align-items: center; z-index: 9999; padding: 16px;
    }
    .login-box {
      background: rgba(30, 30, 32, 0.75); border: 1px solid var(--card-border); border-radius: 24px;
      padding: 28px; width: 100%; max-width: 380px; text-align: center; box-shadow: 0 20px 50px rgba(0,0,0,0.9);
      backdrop-filter: blur(20px); -webkit-backdrop-filter: blur(20px);
    }
    .input-group { margin-bottom: 14px; text-align: left; }
    .input-group label { display: block; font-size: 0.75rem; color: var(--muted); margin-bottom: 6px; text-transform: uppercase; letter-spacing: 0.5px; font-weight: 600; }
    .input-wrapper { position: relative; display: flex; align-items: center; }
    .input-wrapper input {
      width: 100%; padding: 12px 14px; border-radius: 12px; border: 1px solid var(--card-border);
      background: rgba(0, 0, 0, 0.4); color: var(--text); outline: none; font-size: 0.95rem; transition: all 0.2s;
    }
    .input-wrapper input:focus { border-color: var(--accent); background: rgba(0, 0, 0, 0.6); }
    .eye-btn { position: absolute; right: 12px; background: none; border: none; color: var(--muted); cursor: pointer; font-size: 1.1rem; }

    header {
      background: rgba(20, 20, 22, 0.7); backdrop-filter: blur(20px); -webkit-backdrop-filter: blur(20px); border-bottom: 1px solid var(--card-border);
      padding: 12px 16px; display: flex; justify-content: space-between; align-items: center; gap: 10px;
      position: sticky; top: 0; z-index: 100;
    }

    .nav-tabs { 
      display: flex; gap: 6px; background: rgba(120, 120, 128, 0.16); padding: 4px; border-radius: 12px;
      overflow-x: auto; scrollbar-width: none; -webkit-overflow-scrolling: touch; flex: 1; max-width: 100%;
    }
    .nav-tabs::-webkit-scrollbar { display: none; }
    .tab-btn { 
      padding: 8px 14px; border: none; background: transparent; color: var(--muted); border-radius: 9px; 
      cursor: pointer; font-weight: 500; font-size: 0.82rem; white-space: nowrap; transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1); flex-shrink: 0; 
    }
    .tab-btn.active { background: rgba(255, 255, 255, 0.15); color: #fff; font-weight: 600; box-shadow: 0 2px 8px rgba(0,0,0,0.2); }
    .btn-logout { background: rgba(255, 69, 58, 0.15); color: var(--err); border: 1px solid rgba(255, 69, 58, 0.25); padding: 8px 14px; border-radius: 10px; cursor: pointer; font-size: 0.8rem; font-weight: 600; flex-shrink: 0; transition: 0.2s; }
    .btn-logout:active { transform: scale(0.96); }

    .container { max-width: 1120px; margin: 16px auto; padding: 0 14px; }
    .tab-content { display: none; }
    .tab-content.active { display: block; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 14px; margin-bottom: 14px; }
    
    .card { 
      background: var(--card); border: 1px solid var(--card-border); border-radius: 20px; padding: 16px; 
      backdrop-filter: blur(20px); -webkit-backdrop-filter: blur(20px);
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3); transition: transform 0.2s, box-shadow 0.2s;
    }
    .card-title { font-size: 0.78rem; color: var(--muted); margin-bottom: 8px; display: flex; justify-content: space-between; align-items: center; text-transform: uppercase; letter-spacing: 0.6px; font-weight: 600; }
    .val-large { font-size: 1.8rem; font-weight: 700; letter-spacing: -0.5px; }

    .moisture-card-apple {
      position: relative; background: linear-gradient(145deg, rgba(32,32,35,0.7), rgba(20,20,22,0.8));
      border: 1px solid var(--card-border); border-radius: 20px; padding: 16px; display: flex;
      flex-direction: column; justify-content: space-between; gap: 12px; overflow: hidden;
      backdrop-filter: blur(15px); -webkit-backdrop-filter: blur(15px); transition: all 0.3s ease;
    }
    .moisture-badge-apple {
      display: inline-flex; align-items: center; gap: 6px; padding: 6px 12px; border-radius: 30px;
      font-size: 0.8rem; font-weight: 600; letter-spacing: -0.2px; transition: all 0.3s ease; width: max-content;
    }
    .moisture-badge-wet { background: rgba(48, 209, 88, 0.15); color: #30d158; border: 1px solid rgba(48, 209, 88, 0.3); }
    .moisture-badge-dry { background: rgba(255, 159, 10, 0.15); color: #ff9f0a; border: 1px solid rgba(255, 159, 10, 0.3); }

    .badge { font-size: 0.72rem; padding: 4px 9px; border-radius: 8px; font-weight: 600; display: inline-flex; align-items: center; gap: 4px; }
    .badge-ok { background: rgba(48, 209, 88, 0.15); color: var(--ok); border: 1px solid rgba(48, 209, 88, 0.25); }
    .badge-err { background: rgba(255, 69, 58, 0.15); color: var(--err); border: 1px solid rgba(255, 69, 58, 0.25); }
    .badge-warn { background: rgba(255, 214, 10, 0.15); color: var(--warn); border: 1px solid rgba(255, 214, 10, 0.25); }
    .badge-info { background: rgba(10, 132, 255, 0.15); color: var(--accent); border: 1px solid rgba(10, 132, 255, 0.25); }

    .ios-switch { position: relative; display: inline-block; width: 48px; height: 28px; flex-shrink: 0; }
    .ios-switch input { opacity: 0; width: 0; height: 0; }
    .slider { position: absolute; cursor: pointer; inset: 0; background-color: rgba(120, 120, 128, 0.32); transition: .3s; border-radius: 34px; }
    .slider:before { position: absolute; content: ""; height: 22px; width: 22px; left: 3px; bottom: 3px; background-color: white; transition: .3s; border-radius: 50%; }
    input:checked + .slider { background-color: var(--ok); }
    input:checked + .slider:before { transform: translateX(20px); }

    .btn { width: 100%; padding: 12px; border: 1px solid var(--card-border); border-radius: 12px; background: rgba(255,255,255,0.08); color: var(--text); font-weight: 600; cursor: pointer; margin-top: 8px; transition: all 0.2s; font-size: 0.88rem; }
    .btn:active { transform: scale(0.96); }
    .btn.active { background: var(--accent); color: #fff; border: none; box-shadow: 0 4px 14px rgba(10, 132, 255, 0.4); }
    .btn.danger { background: rgba(255, 69, 58, 0.2); color: var(--err); border: 1px solid var(--err); }
    
    input[type="color"] { border: none; width: 38px; height: 38px; border-radius: 10px; cursor: pointer; background: none; }
    input[type="range"] { width: 100%; accent-color: var(--accent); }
    select, input[type="text"], input[type="number"] { background: rgba(0, 0, 0, 0.4); color: var(--text); border: 1px solid var(--card-border); border-radius: 10px; padding: 10px; font-size: 0.85rem; width: 100%; outline: none; }
    table { width: 100%; border-collapse: collapse; margin-top: 8px; font-size: 0.82rem; }
    th, td { padding: 12px 10px; text-align: left; border-bottom: 1px solid var(--card-border); }
    th { color: var(--muted); font-size: 0.72rem; text-transform: uppercase; font-weight: 600; }
    
    .log-box { background: rgba(0, 0, 0, 0.6); border: 1px solid var(--card-border); border-radius: 14px; padding: 14px; font-family: monospace; font-size: 0.78rem; height: 220px; overflow-y: auto; color: #a5b4fc; }
    .clock-card { background: linear-gradient(135deg, rgba(255,255,255,0.05), rgba(28,28,30,0.8)); border: 1px solid var(--card-border); }
    .progress-bar { width: 100%; background: rgba(255, 255, 255, 0.1); height: 8px; border-radius: 6px; overflow: hidden; margin-top: 6px; }
    .progress-fill { height: 100%; background: var(--accent); width: 0%; transition: width 0.4s ease; }

    .modal { position: fixed; inset: 0; background: rgba(0,0,0,0.8); backdrop-filter: blur(15px); display: none; justify-content: center; align-items: center; z-index: 10000; padding: 16px; }
    .modal-box { background: rgba(30, 30, 32, 0.9); border: 1px solid var(--card-border); border-radius: 20px; padding: 24px; max-width: 340px; width: 100%; text-align: center; }

    #pwaInstallBanner {
      position: fixed; bottom: 20px; right: 20px; z-index: 999;
      background: var(--accent); color: #fff; padding: 12px 20px; border-radius: 30px;
      font-size: 0.85rem; font-weight: 600; box-shadow: 0 8px 25px rgba(10, 132, 255, 0.5);
      display: none; cursor: pointer; align-items: center; gap: 8px;
    }
  </style>
</head>
<body>

  <div id="pwaInstallBanner" onclick="downloadAPK()">📲 Download / Install APK</div>

  <!-- LOGIN OVERLAY -->
  <div id="loginOverlay">
    <div class="login-box">
      <h3 style="color:var(--text); margin-bottom:18px; font-weight:700; font-size:1.2rem;">System Login</h3>
      <div class="input-group">
        <label>Username</label>
        <div class="input-wrapper"><input type="text" id="username"></div>
      </div>
      <div class="input-group">
        <label>Password</label>
        <div class="input-wrapper">
          <input type="password" id="password">
          <button type="button" class="eye-btn" onclick="togglePasswordView()">👁️</button>
        </div>
      </div>
      <div class="input-group">
        <label>Google Authenticator Code (2FA)</label>
        <div class="input-wrapper">
          <input type="text" id="totpCode" placeholder="6-Digit Code" maxlength="6" style="letter-spacing:3px; text-align:center; font-weight:700; font-size:1.1rem;">
        </div>
      </div>
      <div style="display:flex; justify-content:flex-start; align-items:center; margin-bottom:18px; font-size:0.85rem; color:var(--muted);">
        <label style="display:flex; align-items:center; gap:6px; cursor:pointer;">
          <input type="checkbox" id="rememberMe"> Remember Me
        </label>
      </div>
      <button class="btn active" onclick="attemptLogin()">Log In</button>
      <div id="loginError" style="color:var(--err); font-size:0.8rem; margin-top:12px; display:none;">Invalid credentials or 2FA Code!</div>
    </div>
  </div>

  <!-- EXTRAS MODAL (DOWNLOADS & 2FA QR) -->
  <div id="qrModal" class="modal">
    <div class="modal-box">
      <h4 style="margin-bottom:12px; color:var(--text);">Other Settings</h4>
      
      <!-- Direct GitHub Raw APK Download Link -->
      <button class="btn active" style="margin-bottom:16px; background:rgba(48, 209, 88, 0.8);" onclick="downloadAPK()">📲 Download Android APK</button>
      
      <div style="border-top: 1px solid var(--card-border); padding-top: 12px;">
        <p style="font-size:0.78rem; color:var(--muted); margin-bottom:10px;">Google Authenticator Pairing QR Code:</p>
        <canvas id="qrCanvas" style="border-radius:12px; background:#fff; padding:8px;"></canvas>
      </div>
      
      <button class="btn" style="margin-top:16px;" onclick="closeQRModal()">Close</button>
    </div>
  </div>

  <header>
    <div class="nav-tabs">
      <button class="tab-btn active" onclick="switchTab('dashboardTab', this)">Dashboard</button>
      <button class="tab-btn" onclick="switchTab('seqIrrigationTab', this)">Sequential Engine</button>
      <button class="tab-btn" onclick="switchTab('automationTab', this)">Automation</button>
      <button class="tab-btn" onclick="switchTab('historianTab', this)">Historian & Logs</button>
    </div>
    <div style="display:flex; gap:6px;">
      <button class="tab-btn" style="background:rgba(10, 132, 255, 0.2); border:1px solid rgba(10, 132, 255, 0.4); color:var(--accent);" onclick="openQRModal()" title="Extras">Extras</button>
      <button class="btn-logout" onclick="logout()">Logout</button>
    </div>
  </header>

  <div class="container">
    
    <div id="seqActiveCard" class="card" style="display:none; border-color:var(--accent); margin-bottom:14px; background: linear-gradient(135deg, rgba(10, 132, 255, 0.15), rgba(28,28,30,0.9));">
      <div class="card-title" style="color:var(--accent);">
        <span>🔴 SEQUENTIAL IRRIGATION IN PROGRESS</span>
        <button class="btn danger" style="width:auto; padding:5px 12px; margin:0;" onclick="stopSequentialIrrigationWithConfirm()">STOP</button>
      </div>
      <div style="display:flex; justify-content:space-between; align-items:center; margin-top:8px;">
        <span style="font-size:1rem; font-weight:600;" id="seqCurrentStep">Active Line: Loading...</span>
        <span style="font-size:1.2rem; font-weight:700; color:var(--accent);" id="seqTimeRemaining">--:--</span>
      </div>
      <div class="progress-bar" style="margin-top:10px; height:8px;"><div class="progress-fill" id="seqProgressFill"></div></div>
    </div>

    <div id="dashboardTab" class="tab-content active">
      <div class="card clock-card" style="margin-bottom: 14px; display:flex; justify-content:space-between; align-items:center;">
        <div>
          <div style="font-size:0.7rem; color:var(--muted); letter-spacing: 1px; font-weight: 600;">LIVE SYSTEM TIME</div>
          <div id="liveDate" style="font-size:0.95rem; font-weight:500; color:var(--text); margin-top: 2px;">-- --- ----</div>
        </div>
        <div id="liveClock" style="font-size:2rem; font-weight:700; color:#fff; letter-spacing: -1px;">00:00:00</div>
      </div>

      <div class="grid">
        <div class="card">
          <div class="card-title">Weather Info <span id="weatherIcon">🌤️</span></div>
          <div class="val-large" id="weatherTemp">-- °C</div>
          <div style="font-size:0.8rem; color:var(--muted); margin-top: 4px;" id="weatherDesc">Fetching Data...</div>
          <div style="font-size:0.75rem; color:var(--accent); margin-top: 2px;" id="weatherWind">Wind: -- km/h</div>
        </div>
        
        <div class="card">
          <div class="card-title">System Load ⚙️</div>
          <div style="font-size:0.8rem; margin-bottom: 4px;">CPU: <b id="sysCpu">-- %</b>
            <div class="progress-bar"><div class="progress-fill" id="cpuFill"></div></div>
          </div>
          <div style="font-size:0.8rem; margin-bottom: 4px;">RAM: <b id="sysHeap">-- %</b>
            <div class="progress-bar"><div class="progress-fill" id="ramFill"></div></div>
          </div>
          <div style="font-size:0.75rem; color:var(--muted); margin-top: 6px;">Chip: <b id="sysTemp" style="color:var(--warn);">-- °C</b> | Uptime: <b id="sysUptime">--</b></div>
        </div>
        
        <div class="card">
          <div class="card-title">Cloudflare & DDNS 🌐</div>
          <div style="font-size:0.9rem; font-weight:700; color:var(--accent); word-break: break-all;">yourdomain.example.com</div>
          <div style="font-size:0.8rem; margin-top: 6px;">External IP: <b id="ddnsIp">---.---.---.---</b></div>
          <div style="margin-top: 6px;" id="cfStatusBadge"><span class="badge badge-ok">🟢 Tunnel Active</span></div>
        </div>
      </div>

      <div class="grid">
        <div class="card">
          <div class="card-title">Ambient Temp & Humidity</div>
          <div class="val-large"><span id="temp">--</span> °C</div>
          <div style="color:var(--muted); font-size:0.82rem; margin-top: 4px;">Relative Humidity: %<span id="hum">--</span> <span id="dhtStatus" style="margin-left:6px;"></span></div>
        </div>
        
        <div class="card">
          <div class="card-title">Light & Panel Status</div>
          <div style="display:flex; justify-content:space-between; align-items:center; margin-top:6px;">
            <span style="font-size:0.85rem;">LDR Sensor:</span>
            <span id="ldrBadge"><span class="badge badge-info">☀️ DAYTIME</span></span>
          </div>
          <div style="display:flex; justify-content:space-between; align-items:center; margin-top:10px;">
            <span style="font-size:0.85rem;">Manual Button:</span>
            <span id="panoBtnBadge"><span class="badge badge-info">IDLE</span></span>
          </div>
        </div>
      </div>

      <div class="card-title" style="margin: 18px 0 10px 0;">Valve & Relay Controls</div>
      <div class="grid" id="relayGrid"></div>

      <div class="card-title" style="margin: 20px 0 10px 0;">Digital Soil Moisture Sensors</div>
      <div class="grid" id="nemGrid" style="grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));"></div>

      <div class="card-title" style="margin: 20px 0 10px 0;">Lighting & Auxiliary FET</div>
      <div class="grid">
        <div class="card" style="display:flex; justify-content:space-between; align-items:center;">
          <div class="card-title" style="margin:0;">Enclosure LED</div>
          <label class="ios-switch">
            <input type="checkbox" id="switchPanoLed" onchange="togglePanoLed()">
            <span class="slider"></span>
          </label>
        </div>
        <div class="card" style="display:flex; justify-content:space-between; align-items:center;">
          <div class="card-title" style="margin:0;">Secondary FET</div>
          <label class="ios-switch">
            <input type="checkbox" id="switchIkinciFet" onchange="toggleIkinciFet()">
            <span class="slider"></span>
          </label>
        </div>
        <div class="card" style="grid-column: span 1 / -1;">
          <div class="card-title">
            WLED Garden Strip
            <label class="ios-switch">
              <input type="checkbox" id="switchWled" onchange="toggleWLED()">
              <span class="slider"></span>
            </label>
          </div>
          <div style="display:flex; gap:10px; margin-top:10px; margin-bottom:10px; align-items:center;">
            <input type="color" id="wledColorPicker" onchange="setWLEDColor(this.value)">
            <select id="wledFxSelect" onchange="setWLEDEffect(this.value)">
              <option value="0">Solid</option>
              <option value="1">Blink</option>
              <option value="2">Breathe</option>
              <option value="9">Wipe</option>
              <option value="101">Pacific Wave (Water Effect)</option>
            </select>
          </div>
          <input type="range" id="wledBriSlider" min="0" max="255" onchange="setWLEDBrightness(this.value)">
        </div>
      </div>
    </div>

    <!-- SEQUENTIAL IRRIGATION TAB -->
    <div id="seqIrrigationTab" class="tab-content">
      <div class="card" style="margin-bottom:14px;">
        <div class="card-title">Sequential Irrigation Settings (Relay 1 - Relay 7)</div>
        <p style="font-size:0.8rem; color:var(--muted); margin-bottom:12px;">Set the default runtime in minutes for each valve line. Check <b>"Skip"</b> to bypass specific relays during execution.</p>
        
        <div id="seqDurationInputs" class="grid" style="grid-template-columns: repeat(auto-fit, minmax(130px, 1fr));"></div>
        
        <div style="margin-top:14px; display:flex; gap:10px; flex-wrap:wrap;">
          <div style="flex:1; min-width:180px;">
            <label style="font-size:0.75rem; color:var(--muted); display:block; margin-bottom:4px;">SEASONAL MULTIPLIER</label>
            <select id="seqMultiplier">
              <option value="0.5">50% (Rainy / Cool)</option>
              <option value="1.0" selected>100% (Normal Duration)</option>
              <option value="1.5">150% (Hot / Dry)</option>
            </select>
          </div>
          <div style="flex:1; min-width:180px;">
            <label style="font-size:0.75rem; color:var(--muted); display:block; margin-bottom:4px;">IRRIGATION MODE 💧</label>
            <select id="seqDualMode">
              <option value="false" selected>Single Line (1 Valve at a time)</option>
              <option value="true">Dual Line (2 Valves simultaneously)</option>
            </select>
          </div>
          <div style="flex:1; min-width:200px; display:flex; gap:8px; align-items:flex-end;">
            <button class="btn" style="flex:1; margin:0;" onclick="saveRelayTimesEEPROM()">Save 💾</button>
            <button class="btn active" style="flex:1.5; margin:0;" onclick="startSequentialIrrigationWithConfirm()">Start 🚀</button>
          </div>
        </div>
      </div>
    </div>

    <!-- AUTOMATION TAB -->
    <div id="automationTab" class="tab-content">
      <div class="card" style="margin-bottom:14px; border-color: rgba(10, 132, 255, 0.3);">
        <div class="card-title" style="color:var(--accent);">
          <span>WLED Night Automation (Warm White) 🌙</span>
          <label class="ios-switch">
            <input type="checkbox" id="wledNightAuto" onchange="saveWledNightConfig()">
            <span class="slider"></span>
          </label>
        </div>
        <p style="font-size:0.8rem; color:var(--muted); margin-bottom:12px;">Automatically turns on warm ambient lighting when LDR detects darkness at night, and turns off at dawn.</p>
        
        <div style="display:grid; grid-template-columns: repeat(auto-fit, minmax(130px, 1fr)); gap:10px;">
          <div>
            <label style="font-size:0.72rem; color:var(--muted); display:block; margin-bottom:4px;">Night Brightness</label>
            <input type="range" id="wledNightBri" min="10" max="255" value="100" onchange="saveWledNightConfig()">
          </div>
        </div>
      </div>

      <div class="card" style="margin-bottom:14px;">
        <div class="card-title">Add Custom Automation Rule</div>
        <div style="display:grid; grid-template-columns: repeat(auto-fit, minmax(130px, 1fr)); gap:8px; margin-top:10px;">
          <select id="autoType">
            <option value="time">Time Trigger (HH:MM)</option>
            <option value="nem_kuru">Soil Sensor Dry</option>
            <option value="temp_high">Temperature ></option>
            <option value="ldr_night">LDR Night Detected</option>
          </select>
          <select id="autoTarget">
            <option value="0">Relay 1</option><option value="1">Relay 2</option><option value="2">Relay 3</option><option value="3">Relay 4</option>
            <option value="4">Relay 5</option><option value="5">Relay 6</option><option value="6">Relay 7</option>
            <option value="seq_all">Start Sequential Irrigation</option>
          </select>
          <input type="text" id="autoValue" placeholder="Threshold / Time">
          <select id="autoAction">
            <option value="ON">TURN ON</option>
            <option value="OFF">TURN OFF</option>
          </select>
          <button class="btn active" onclick="addAutomationRule()" style="margin:0;">Add Rule</button>
        </div>
      </div>
      <div class="card" style="overflow-x:auto;">
        <div class="card-title">Active Rules</div>
        <table>
          <thead><tr><th>Trigger</th><th>Target</th><th>Value</th><th>Action</th><th>Manage</th></tr></thead>
          <tbody id="automationTableBody"></tbody>
        </table>
      </div>
    </div>

    <!-- HISTORIAN & SYSTEM LOGS -->
    <div id="historianTab" class="tab-content">
      <div class="grid">
        <div class="card">
          <div class="card-title" style="display:flex; justify-content:space-between;">
            <span>DHT11 Telemetry History</span>
            <button onclick="clearHistorian()" style="background:none; border:none; color:var(--err); cursor:pointer; font-size:0.7rem;">Clear</button>
          </div>
          <div style="height:220px;"><canvas id="tempHumChart"></canvas></div>
        </div>
        <div class="card">
          <div class="card-title">Resource Utilization (CPU & RAM)</div>
          <div style="height:220px;"><canvas id="cpuChart"></canvas></div>
        </div>
      </div>
      
      <div class="card-title" style="margin: 16px 0 8px 0;">Live System Terminal Logs</div>
      <div class="log-box" id="logTerminal">Fetching system logs...</div>
    </div>
  </div>

  <script>
    let tempChart, cpuChart;
    let initialTimesLoaded = false;
    let lastNotifyStep = "";
    let deferredPrompt;

    // Download APK directly from GitHub repository raw release path
    function downloadAPK() {
      window.location.href = "https://raw.githubusercontent.com/alinguuu/ESP32-SmartIrrigation/main/app-release.apk";
    }

    window.addEventListener('beforeinstallprompt', (e) => {
      e.preventDefault();
      deferredPrompt = e;
      document.getElementById('pwaInstallBanner').style.display = 'flex';
    });

    if ("Notification" in window && Notification.permission !== "granted") {
      Notification.requestPermission();
    }

    function sendNativeNotification(title, body) {
      if ("Notification" in window && Notification.permission === "granted") {
        new Notification(title, { 
          body: body, 
          icon: 'data:image/svg+xml,<svg xmlns=\'http://www.w3.org/2000/svg\' viewBox=\'0 0 100 100\'><text y=\'.9em\' font-size=\'90\'>🌿</text></svg>' 
        });
      }
    }

    function togglePasswordView() {
      const p = document.getElementById("password");
      p.type = p.type === "password" ? "text" : "password";
    }

    function openQRModal() {
      document.getElementById("qrModal").style.display = "flex";
      fetch('/get2FAKey').then(r => r.json()).then(d => {
        new QRious({
          element: document.getElementById('qrCanvas'),
          value: `otpauth://totp/SmartIrrigation:admin?secret=${d.secret}&issuer=SmartIrrigation`,
          size: 180
        });
      });
    }

    function closeQRModal() { document.getElementById("qrModal").style.display = "none"; }

    function updateClock() {
      const now = new Date();
      document.getElementById('liveClock').innerText = now.toLocaleTimeString('en-US');
      const options = { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' };
      document.getElementById('liveDate').innerText = now.toLocaleDateString('en-US', options);
    }
    setInterval(updateClock, 1000);

    function checkAuth() {
      const token = localStorage.getItem("system_token") || sessionStorage.getItem("system_token");
      if (token === "authenticated") {
        document.getElementById("loginOverlay").style.display = "none";
        initDashboard();
      } else {
        document.getElementById("loginOverlay").style.display = "flex";
      }
    }

    function attemptLogin() {
      const u = document.getElementById("username").value;
      const p = document.getElementById("password").value;
      const totp = document.getElementById("totpCode").value;

      fetch(`/login?u=${encodeURIComponent(u)}&p=${encodeURIComponent(p)}&code=${encodeURIComponent(totp)}`)
        .then(r => r.json())
        .then(res => {
          if (res.status === "OK") {
            (document.getElementById("rememberMe").checked ? localStorage : sessionStorage).setItem("system_token", "authenticated");
            document.getElementById("loginOverlay").style.display = "none";
            initDashboard();
          } else {
            document.getElementById("loginError").style.display = "block";
          }
        })
        .catch(() => { document.getElementById("loginError").style.display = "block"; });
    }

    function logout() { localStorage.clear(); sessionStorage.clear(); location.reload(); }

    function switchTab(tabId, btn) {
      document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
      document.querySelectorAll('.tab-btn').forEach(el => el.classList.remove('active'));
      document.getElementById(tabId).classList.add('active');
      btn.classList.add('active');
    }

    function initDashboard() {
      updateClock(); renderRelayCards(); renderNemCards(); renderSeqInputs(); initCharts(); fetchWeather(); fetchStatus(); fetchWledConfig(); fetchRules();
      setInterval(fetchStatus, 2000); setInterval(fetchLogs, 4000); setInterval(fetchWeather, 600000);
    }

    function renderRelayCards() {
      let h = "";
      for (let i = 0; i < 8; i++) {
        let label = (i === 7) ? "Relay 8 (Aux / Spare)" : `Relay ${i+1}`;
        h += `<div class="card" style="display:flex; justify-content:space-between; align-items:center;">
                <div>
                  <div class="card-title" style="margin:0;">${label}</div>
                </div>
                <label class="ios-switch">
                  <input type="checkbox" id="switchRelay${i}" onchange="toggleRelayWithConfirm(${i})">
                  <span class="slider"></span>
                </label>
              </div>`;
      }
      document.getElementById("relayGrid").innerHTML = h;
    }

    function renderNemCards() {
      let h = "";
      for (let i = 0; i < 4; i++) {
        h += `<div class="moisture-card-apple">
                <div style="display:flex; justify-content:space-between; align-items:center;">
                  <span style="font-size:0.78rem; color:var(--muted); font-weight:600; text-transform:uppercase;">Digital Sensor ${i+1}</span>
                </div>
                <div id="nemVal${i}">
                  <div class="moisture-badge-apple moisture-badge-dry">...</div>
                </div>
              </div>`;
      }
      document.getElementById("nemGrid").innerHTML = h;
    }

    function renderSeqInputs() {
      let h = "";
      for (let i = 0; i < 7; i++) {
        h += `<div class="card" style="padding:10px;">
                <div style="display:flex; justify-content:space-between; align-items:center;">
                  <label style="font-size:0.72rem; color:var(--muted); font-weight:600;">Relay ${i+1} (min)</label>
                  <label style="font-size:0.68rem; color:var(--warn); cursor:pointer;">
                    <input type="checkbox" id="seqSkip${i}"> Skip
                  </label>
                </div>
                <input type="number" id="seqDur${i}" value="10" min="1" max="120" style="margin-top:6px;">
              </div>`;
      }
      document.getElementById("seqDurationInputs").innerHTML = h;
    }

    function fetchStatus() {
      fetch('/status').then(res => res.json()).then(data => {
        const dhtError = (data.temp === -999 || data.hum === -999 || isNaN(data.temp));
        
        if (dhtError) {
          document.getElementById('temp').innerText = "ERR";
          document.getElementById('hum').innerText = "ERR";
          document.getElementById('dhtStatus').innerHTML = "<span class='badge badge-err'>⚠️ ERR</span>";
        } else {
          document.getElementById('temp').innerText = data.temp;
          document.getElementById('hum').innerText = data.hum;
          document.getElementById('dhtStatus').innerHTML = "<span class='badge badge-ok'>●</span>";
        }
        
        data.relays.forEach((st, i) => {
          const sw = document.getElementById(`switchRelay${i}`);
          if (sw) sw.checked = st;
        });

        data.nem.forEach((st, i) => {
          const el = document.getElementById(`nemVal${i}`);
          if (el) {
            el.innerHTML = st 
              ? `<div class="moisture-badge-apple moisture-badge-wet">💧 WET</div>` 
              : `<div class="moisture-badge-apple moisture-badge-dry">🌵 DRY</div>`;
          }
        });

        document.getElementById('ldrBadge').innerHTML = data.ldr 
          ? `<span class="badge badge-warn">🌙 NIGHT</span>` 
          : `<span class="badge badge-info">☀️ DAYTIME</span>`;
          
        document.getElementById('panoBtnBadge').innerHTML = data.pano_btn 
          ? `<span class="badge badge-err">PRESSED</span>` 
          : `<span class="badge badge-info">IDLE</span>`;
        
        document.getElementById('switchPanoLed').checked = data.pano_led;
        document.getElementById('switchIkinciFet').checked = data.ikinci_fet;
        
        if (data.wled) {
          document.getElementById('switchWled').checked = data.wled.state;
          if (data.wled.color) document.getElementById('wledColorPicker').value = data.wled.color;
          if (data.wled.bri !== undefined) document.getElementById('wledBriSlider').value = data.wled.bri;
        }
        
        document.getElementById('sysTemp').innerText = data.sys_temp + " °C";
        document.getElementById('sysCpu').innerText = data.sys_cpu + "%";
        document.getElementById('cpuFill').style.width = data.sys_cpu + "%";
        
        document.getElementById('sysHeap').innerText = data.sys_heap + "%";
        document.getElementById('ramFill').style.width = data.sys_heap + "%";
        
        document.getElementById('sysUptime').innerText = data.sys_uptime;
        document.getElementById('ddnsIp').innerText = data.ddns_ip;

        if (data.relay_times && !initialTimesLoaded) {
          data.relay_times.forEach((timeVal, idx) => {
            const inputEl = document.getElementById(`seqDur${idx}`);
            if (inputEl) inputEl.value = timeVal;
          });
          initialTimesLoaded = true;
        }

        if (data.seq_active) {
          document.getElementById('seqActiveCard').style.display = 'block';
          
          let sec = data.seq_remain_sec || 0;
          let m = Math.floor(sec / 60);
          let s = sec % 60;
          let timeFormatted = `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
          
          let stepTxt = `Active Line: Relay ${data.seq_relay + 1}`;
          if (data.seq_dual) {
            stepTxt = `Active Lines: Relay ${data.seq_relay + 1} & Relay ${data.seq_relay + 2}`;
          }
          document.getElementById('seqCurrentStep').innerText = stepTxt;
          document.getElementById('seqTimeRemaining').innerText = timeFormatted;

          if (lastNotifyStep !== stepTxt) {
            sendNativeNotification("💧 Irrigation Step Started", `${stepTxt} (Duration: ${Math.ceil(data.seq_total_sec / 60)} min)`);
            lastNotifyStep = stepTxt;
          }
          
          let pct = data.seq_total_sec > 0 ? ((data.seq_total_sec - sec) / data.seq_total_sec) * 100 : 0;
          document.getElementById('seqProgressFill').style.width = pct + '%';
        } else {
          if (lastNotifyStep !== "") {
            sendNativeNotification("✅ Irrigation Completed", "Sequential irrigation cycle finished successfully.");
            lastNotifyStep = "";
          }
          document.getElementById('seqActiveCard').style.display = 'none';
        }
        
        if (!dhtError) {
          updateCharts(data.temp, data.hum, data.sys_heap, data.sys_cpu);
        }
      }).catch(()=>{});
    }

    function fetchWledConfig() {
      fetch('/getWLEDConfig').then(r => r.json()).then(conf => {
        document.getElementById('wledNightAuto').checked = conf.auto;
        document.getElementById('wledNightBri').value = conf.bri;
      }).catch(()=>{});
    }

    function saveWledNightConfig() {
      const auto = document.getElementById('wledNightAuto').checked;
      const bri = document.getElementById('wledNightBri').value;
      fetch(`/setWLEDConfig?auto=${auto}&bri=${bri}`);
    }

    function saveRelayTimesEEPROM() {
      let durArray = [];
      for (let i = 0; i < 7; i++) {
        durArray.push(document.getElementById(`seqDur${i}`).value || 10);
      }
      fetch(`/setRelayTimes?times=${durArray.join(',')}`).then(() => alert("Timers saved successfully."));
    }

    function toggleRelayWithConfirm(i) { fetch(`/toggleRelay?id=${i}`); }

    function startSequentialIrrigationWithConfirm() {
      if (confirm("Start sequential irrigation cycle?")) {
        let durArray = [];
        let skips = [];
        for (let i = 0; i < 7; i++) {
          durArray.push(document.getElementById(`seqDur${i}`).value || 10);
          skips.push(document.getElementById(`seqSkip${i}`).checked ? "1" : "0");
        }
        const mult = document.getElementById('seqMultiplier').value;
        const dual = document.getElementById('seqDualMode').value;
        fetch(`/startSequence?times=${durArray.join(',')}&skips=${skips.join(',')}&mult=${mult}&dual=${dual}`);
      }
    }

    function stopSequentialIrrigationWithConfirm() {
      if (confirm("Are you sure you want to STOP the irrigation process?")) {
        fetch('/stopSequence');
      }
    }

    function togglePanoLed() { fetch('/togglePanoLed'); }
    function toggleIkinciFet() { fetch('/toggleIkinciFet'); }
    function toggleWLED() { fetch(`/toggleWLED?state=${document.getElementById('switchWled').checked}`); }
    function setWLEDBrightness(v) { fetch(`/setWLEDBrightness?bri=${v}`); }
    function setWLEDEffect(v) { fetch(`/setWLEDEffect?fx=${v}`); }
    function setWLEDColor(hex) { fetch(`/setWLEDColor?hex=${encodeURIComponent(hex)}`); }

    function fetchLogs() {
      fetch('/getLogs').then(r => r.json()).then(l => {
        document.getElementById('logTerminal').innerHTML = l.join('<br>');
      }).catch(()=>{});
    }

    function fetchWeather() {
      // Default Open-Meteo Coordinates (Adjust if necessary)
      fetch('https://api.open-meteo.com/v1/forecast?latitude=38.65&longitude=35.45&current_weather=true')
        .then(r => r.json()).then(d => {
          document.getElementById('weatherTemp').innerText = `${d.current_weather.temperature} °C`;
          document.getElementById('weatherWind').innerText = `Wind: ${d.current_weather.windspeed} km/h`;
          document.getElementById('weatherDesc').innerText = getWeatherDescription(d.current_weather.weathercode);
        }).catch(()=>{ document.getElementById('weatherDesc').innerText = "Weather Unavailable"; });
    }

    function getWeatherDescription(c) {
      if (c === 0) return "Clear Sky";
      if (c <= 3) return "Partly Cloudy";
      if (c <= 48) return "Foggy";
      if (c <= 67) return "Rainy";
      return "Snowy";
    }

    function initCharts() {
      const commonOpts = { responsive: true, maintainAspectRatio: false, plugins: { legend: { labels: { color: '#8e8e93' } } } };
      
      const savedHistory = JSON.parse(localStorage.getItem("irrigation_historian_data") || "{\"labels\":[],\"temp\":[],\"hum\":[],\"cpu\":[],\"ram\":[]}");

      tempChart = new Chart(document.getElementById('tempHumChart').getContext('2d'), {
        type: 'line', 
        data: { 
          labels: savedHistory.labels, 
          datasets: [
            { label: 'Temp (°C)', borderColor: '#0a84ff', data: savedHistory.temp, tension:0.3 }, 
            { label: 'Humidity (%)', borderColor: '#30d158', data: savedHistory.hum, tension:0.3 }
          ] 
        }, 
        options: commonOpts
      });

      cpuChart = new Chart(document.getElementById('cpuChart').getContext('2d'), {
        type: 'line', 
        data: { 
          labels: savedHistory.labels, 
          datasets: [
            { label: 'RAM (%)', borderColor: '#ffd60a', backgroundColor:'rgba(255,214,10,0.08)', fill:true, data: savedHistory.ram, tension:0.3 },
            { label: 'CPU (%)', borderColor: '#ff453a', backgroundColor:'transparent', data: savedHistory.cpu, tension:0.3 }
          ] 
        }, 
        options: commonOpts
      });
    }

    function updateCharts(t, h, heap, cpu) {
      const now = new Date().toLocaleTimeString();
      
      let savedHistory = JSON.parse(localStorage.getItem("irrigation_historian_data") || "{\"labels\":[],\"temp\":[],\"hum\":[],\"cpu\":[],\"ram\":[]}");
      
      if (savedHistory.labels.length > 500) {
        savedHistory.labels.shift(); savedHistory.temp.shift(); savedHistory.hum.shift(); savedHistory.cpu.shift(); savedHistory.ram.shift();
      }

      savedHistory.labels.push(now);
      savedHistory.temp.push(t);
      savedHistory.hum.push(h);
      savedHistory.cpu.push(cpu);
      savedHistory.ram.push(heap);

      localStorage.setItem("irrigation_historian_data", JSON.stringify(savedHistory));

      tempChart.data.labels = savedHistory.labels;
      tempChart.data.datasets[0].data = savedHistory.temp;
      tempChart.data.datasets[1].data = savedHistory.hum;
      tempChart.update();

      cpuChart.data.labels = savedHistory.labels;
      cpuChart.data.datasets[0].data = savedHistory.ram;
      cpuChart.data.datasets[1].data = savedHistory.cpu;
      cpuChart.update();
    }

    function clearHistorian() {
      localStorage.removeItem("irrigation_historian_data");
      location.reload();
    }

    function fetchRules() {
      fetch('/getRules').then(r => r.json()).then(rules => renderAutomationRules(rules)).catch(()=>{});
    }

    function addAutomationRule() {
      const type = document.getElementById('autoType').value,
            target = document.getElementById('autoTarget').value,
            val = document.getElementById('autoValue').value,
            action = document.getElementById('autoAction').value;
            
      const url = `/addRule?type=${encodeURIComponent(type)}&target=${encodeURIComponent(target)}&val=${encodeURIComponent(val)}&action=${encodeURIComponent(action)}`;
      fetch(url).then(r => r.text()).then(() => fetchRules());
    }

    function renderAutomationRules(autoRules) {
      let h = "";
      if (Array.isArray(autoRules)) {
        autoRules.forEach(r => {
          let tName = r.target === 'seq_all' ? 'Sequential Mode' : 'Relay ' + (parseInt(r.target)+1);
          h += `<tr>
                  <td>${r.type}</td>
                  <td>${tName}</td>
                  <td>${r.val || '-'}</td>
                  <td><b>${r.action}</b></td>
                  <td><button class="btn-logout" onclick="deleteRule(${r.id})">Delete</button></td>
                </tr>`;
        });
      }
      document.getElementById('automationTableBody').innerHTML = h;
    }

    function deleteRule(id) { fetch(`/deleteRule?id=${id}`).then(() => fetchRules()); }

    window.onload = checkAuth;
  </script>
</body>
</html>
)rawliteral";

#endif