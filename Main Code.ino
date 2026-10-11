#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <math.h>
#define VOLTAGE_PIN 35
#define CURRENT_PIN 34
#define RELAY_PIN   23
#define SDA_PIN     21
#define SCL_PIN     22
#define LCD_COLUMNS 20
#define LCD_ROWS 4
LiquidCrystal_I2C *lcd = nullptr;
const char* AP_SSID = "Smart-Energy-Meter-Pro";
IPAddress AP_IP(192, 168, 4, 1);
IPAddress AP_GATEWAY(192, 168, 4, 1);
IPAddress AP_SUBNET(255, 255, 255, 0);
DNSServer dnsServer;
WebServer server(80);
float VOLT_CAL = 0.65;
float CURR_CAL = 0.055;
float unitPrice = 8.00;
float OVERLOAD_LIMIT_WATTS = 2500.0;
const int ADC_SAMPLES = 400;
const int SAMPLE_DELAY_US = 30;
float voltageRMS = 0.0;
float currentRMS = 0.0;
float activePower = 0.0;
float apparentPower = 0.0;
float reactivePower = 0.0;
float powerFactor = 1.0;
float frequencyHz = 50.0;
float crestFactor = 1.41;
float totalKWh = 0.0;
float dailyKWh = 0.0;
float hourlyRateUnits = 0.0;
float dailyCostEst = 0.0;
float totalBill = 0.0;
float projectedBill = 0.0;
float carbonKg = 0.0;
float treesRequired = 0.0;
float peakPower = 0.0;
float peakCurrent = 0.0;
String gridQuality = "EXCELLENT";
String systemStatus = "NORMAL (SAFE)";
bool relayState = true;
unsigned long lastMeasurement = 0;
unsigned long lastEnergyTime = 0;
unsigned long lastSaveTime = 0;
unsigned long dailyTimer = 0;
unsigned long startTime = 0;
unsigned long lastLcdPageTime = 0;
int currentLcdPage = 0;
const unsigned long LCD_PAGE_INTERVAL = 3000;
const unsigned long MEASUREMENT_INTERVAL = 1000;
const unsigned long SAVE_INTERVAL = 120000;
Preferences preferences;
const char MAIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>IoT Smart Energy Meter - Trio Titans</title>
<style>
  :root {
    --bg-main: #070b14;
    --card-surface: rgba(15, 23, 42, 0.78);
    --card-border: rgba(56, 189, 248, 0.25);
    --cyan: #38bdf8;
    --emerald: #10b981;
    --violet: #a855f7;
    --amber: #f59e0b;
    --rose: #f43f5e;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    font-family: system-ui, -apple-system, sans-serif;
    background: radial-gradient(circle at 50% 0%, #0f2038 0%, #050811 100%);
    color: #f1f5f9;
    padding: 16px;
    min-height: 100vh;
  }
  .container { max-width: 1100px; margin: auto; }
  header {
    background: var(--card-surface);
    backdrop-filter: blur(16px);
    border: 1px solid var(--card-border);
    border-radius: 16px;
    padding: 20px 24px;
    box-shadow: 0 10px 30px rgba(0,0,0,0.6);
    margin-bottom: 16px;
  }
  .header-top {
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 12px;
  }
  .project-title {
    font-size: 24px;
    font-weight: 900;
    letter-spacing: 0.5px;
    background: linear-gradient(90deg, #38bdf8, #818cf8, #c084fc);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
  }
  .team-tag {
    display: inline-block;
    background: linear-gradient(135deg, #0284c7, #4f46e5);
    color: #fff;
    font-size: 11px;
    font-weight: 800;
    padding: 5px 14px;
    border-radius: 30px;
    letter-spacing: 1.2px;
    text-transform: uppercase;
    box-shadow: 0 0 14px rgba(56, 189, 248, 0.4);
  }
  .members-strip {
    margin-top: 14px;
    padding-top: 12px;
    border-top: 1px solid rgba(255,255,255,0.08);
    display: flex;
    gap: 10px;
    flex-wrap: wrap;
    align-items: center;
  }
  .member-pill {
    background: rgba(30, 41, 59, 0.85);
    border: 1px solid rgba(56, 189, 248, 0.25);
    color: #cbd5e1;
    font-size: 12px;
    font-weight: 600;
    padding: 5px 14px;
    border-radius: 8px;
    display: flex;
    align-items: center;
    gap: 6px;
  }
  .member-pill b { color: #38bdf8; font-weight: 800; }
  .control-strip {
    background: var(--card-surface);
    backdrop-filter: blur(12px);
    border: 1px solid var(--card-border);
    border-radius: 14px;
    padding: 16px 20px;
    margin-bottom: 16px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 12px;
  }
  .btn-relay {
    padding: 12px 28px;
    font-weight: 800;
    border: none;
    border-radius: 10px;
    cursor: pointer;
    font-size: 13px;
    letter-spacing: 0.5px;
    transition: 0.3s;
  }
  .relay-on {
    background: linear-gradient(135deg, #10b981, #059669);
    color: white;
    box-shadow: 0 0 16px rgba(16, 185, 129, 0.4);
  }
  .relay-off {
    background: linear-gradient(135deg, #ef4444, #dc2626);
    color: white;
    box-shadow: 0 0 16px rgba(239, 68, 68, 0.4);
  }
  .gauges-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
    gap: 12px;
    margin-bottom: 16px;
  }
  .gauge-card {
    background: var(--card-surface);
    backdrop-filter: blur(12px);
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 14px;
    padding: 16px;
    text-align: center;
    position: relative;
  }
  .gauge-svg { width: 140px; height: 140px; transform: rotate(-90deg); margin: 0 auto; display: block; }
  .gauge-bg { fill: none; stroke: rgba(255,255,255,0.06); stroke-width: 10; }
  .gauge-progress {
    fill: none;
    stroke-width: 10;
    stroke-linecap: round;
    stroke-dasharray: 377;
    stroke-dashoffset: 377;
    transition: stroke-dashoffset 0.6s ease;
  }
  .gauge-data {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -25%);
  }
  .gauge-val { font-size: 22px; font-weight: 800; }
  .gauge-lbl { font-size: 11px; font-weight: 700; color: #94a3b8; text-transform: uppercase; margin-top: 8px; }

  .section-label {
    font-size: 11px;
    font-weight: 800;
    color: #94a3b8;
    text-transform: uppercase;
    letter-spacing: 1.2px;
    margin: 18px 0 10px 4px;
    display: flex;
    align-items: center;
    gap: 8px;
  }
  .section-label::after { content:''; flex:1; height:1px; background:rgba(255,255,255,0.08); }
  .grid-4 { display: grid; grid-template-columns: repeat(auto-fit, minmax(210px, 1fr)); gap: 12px; }
  .card {
    background: var(--card-surface);
    backdrop-filter: blur(12px);
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 12px;
    padding: 14px 16px;
    transition: transform 0.2s, border-color 0.2s;
  }
  .card:hover { transform: translateY(-3px); border-color: rgba(56, 189, 248, 0.4); }
  .card-top { display:flex; justify-content:space-between; align-items:center; }
  .card-id { font-size: 10px; font-weight: 800; color: #64748b; }
  .card-name { font-size: 11px; font-weight: 700; color: #94a3b8; text-transform: uppercase; }
  .val-wrap { margin-top: 8px; display: flex; align-items: baseline; gap: 6px; }
  .val { font-size: 24px; font-weight: 800; color: #ffffff; }
  .unit { font-size: 12px; font-weight: 600; color: #94a3b8; }
  .border-cyan { border-top: 3px solid var(--cyan); }
  .border-emerald { border-top: 3px solid var(--emerald); }
  .border-amber { border-top: 3px solid var(--amber); }
  .border-violet { border-top: 3px solid var(--violet); }
  .border-rose { border-top: 3px solid var(--rose); }
  .chart-box {
    background: var(--card-surface);
    backdrop-filter: blur(12px);
    border: 1px solid var(--card-border);
    border-radius: 14px;
    padding: 18px;
    margin-top: 18px;
  }
  canvas { width: 100%; height: 180px; display: block; }
</style>
</head>
<body>
<div class="container">
  <header>
    <div class="header-top">
      <div>
        <div class="project-title">IOT SMART ENERGY METER</div>
        <span class="team-tag">TEAM: TRIO TITANS</span>
      </div>
      <div style="font-size:13px;color:#94a3b8;">ESP32 UPTIME: <span id="uptime" style="color:#38bdf8;font-weight:800;">0m</span></div>
    </div>
    <div class="members-strip">
      <span style="font-size:11px;font-weight:800;color:#64748b;text-transform:uppercase;">TEAM MEMBERS:</span>
      <div class="member-pill"><span>👤</span> <b>Sanjith</b></div>
      <div class="member-pill"><span>👤</span> <b>Vikram</b></div>
      <div class="member-pill"><span>👤</span> <b>Vishnu</b></div>
    </div>
  </header>
  <div class="control-strip">
    <div>
      <span style="font-size:11px;font-weight:700;color:#94a3b8;text-transform:uppercase;">AC Mains Cut-Off Control</span>
      <div id="relayText" style="font-size:16px;font-weight:800;color:#34d399;margin-top:3px;">ONLINE - AC LOAD ACTIVE</div>
    </div>
    <button id="relayBtn" class="btn-relay relay-on" onclick="toggleRelay()">DISCONNECT LOAD</button>
  </div>
  <div class="gauges-grid">
    <div class="gauge-card border-cyan">
      <svg class="gauge-svg" viewBox="0 0 140 140">
        <circle class="gauge-bg" cx="70" cy="70" r="60"></circle>
        <circle id="g-v-bar" class="gauge-progress" stroke="#38bdf8" cx="70" cy="70" r="60"></circle>
      </svg>
      <div class="gauge-data">
        <div class="gauge-val" id="g-v">0</div>
        <div class="unit">VOLTS</div>
      </div>
      <div class="gauge-lbl">AC Voltage (RMS)</div>
    </div>
    <div class="gauge-card border-emerald">
      <svg class="gauge-svg" viewBox="0 0 140 140">
        <circle class="gauge-bg" cx="70" cy="70" r="60"></circle>
        <circle id="g-i-bar" class="gauge-progress" stroke="#10b981" cx="70" cy="70" r="60"></circle>
      </svg>
      <div class="gauge-data">
        <div class="gauge-val" id="g-i">0.00</div>
        <div class="unit">AMPERES</div>
      </div>
      <div class="gauge-lbl">Load Current (RMS)</div>
    </div>
    <div class="gauge-card border-amber">
      <svg class="gauge-svg" viewBox="0 0 140 140">
        <circle class="gauge-bg" cx="70" cy="70" r="60"></circle>
        <circle id="g-pf-bar" class="gauge-progress" stroke="#f59e0b" cx="70" cy="70" r="60"></circle>
      </svg>
      <div class="gauge-data">
        <div class="gauge-val" id="g-pf">1.00</div>
        <div class="unit">FACTOR</div>
      </div>
      <div class="gauge-lbl">Power Factor</div>
    </div>
  </div>
  <div class="section-label">Section 1: Electrical Fundamentals (1 to 8)</div>
  <div class="grid-4">
    <div class="card border-cyan"><div class="card-top"><span class="card-name">1. Voltage (RMS)</span><span class="card-id">#01</span></div><div class="val-wrap"><span class="val" id="v">0.0</span><span class="unit">V</span></div></div>
    <div class="card border-cyan"><div class="card-top"><span class="card-name">2. Current (RMS)</span><span class="card-id">#02</span></div><div class="val-wrap"><span class="val" id="i">0.00</span><span class="unit">A</span></div></div>
    <div class="card border-emerald"><div class="card-top"><span class="card-name">3. Active Power</span><span class="card-id">#03</span></div><div class="val-wrap"><span class="val" id="p" style="color:#34d399;">0.0</span><span class="unit">W</span></div></div>
    <div class="card border-violet"><div class="card-top"><span class="card-name">4. Apparent (S)</span><span class="card-id">#04</span></div><div class="val-wrap"><span class="val" id="va">0.0</span><span class="unit">VA</span></div></div>
    <div class="card border-violet"><div class="card-top"><span class="card-name">5. Reactive (Q)</span><span class="card-id">#05</span></div><div class="val-wrap"><span class="val" id="var">0.0</span><span class="unit">VAR</span></div></div>
    <div class="card border-amber"><div class="card-top"><span class="card-name">6. Power Factor</span><span class="card-id">#06</span></div><div class="val-wrap"><span class="val" id="pf" style="color:#fbbf24;">1.00</span><span class="unit">PF</span></div></div>
    <div class="card border-cyan"><div class="card-top"><span class="card-name">7. Frequency</span><span class="card-id">#07</span></div><div class="val-wrap"><span class="val" id="f">50.0</span><span class="unit">Hz</span></div></div>
    <div class="card border-amber"><div class="card-top"><span class="card-name">8. Crest Factor</span><span class="card-id">#08</span></div><div class="val-wrap"><span class="val" id="cf">1.41</span></div></div>
  </div>
  <div class="section-label">Section 2: Energy Metrics & Run Rates (9 to 12)</div>
  <div class="grid-4">
    <div class="card border-emerald"><div class="card-top"><span class="card-name">9. Total Units</span><span class="card-id">#09</span></div><div class="val-wrap"><span class="val" id="kwh">0.000</span><span class="unit">kWh</span></div></div>
    <div class="card border-emerald"><div class="card-top"><span class="card-name">10. Today's Units</span><span class="card-id">#10</span></div><div class="val-wrap"><span class="val" id="d_kwh">0.000</span><span class="unit">kWh</span></div></div>
    <div class="card border-cyan"><div class="card-top"><span class="card-name">11. Rate / Hour</span><span class="card-id">#11</span></div><div class="val-wrap"><span class="val" id="hr_rate">0.000</span><span class="unit">u/h</span></div></div>
    <div class="card border-amber"><div class="card-top"><span class="card-name">12. Est. Daily Cost</span><span class="card-id">#12</span></div><div class="val-wrap"><span class="unit">Rs.</span><span class="val" id="d_cost">0.00</span></div></div>
  </div>
  <div class="section-label">Section 3: Billing Analytics & Eco Offset (13 to 16)</div>
  <div class="grid-4">
    <div class="card border-emerald"><div class="card-top"><span class="card-name">13. Current Bill</span><span class="card-id">#13</span></div><div class="val-wrap"><span class="unit">Rs.</span><span class="val" id="bill" style="color:#34d399;">0.00</span></div></div>
    <div class="card border-violet"><div class="card-top"><span class="card-name">14. Projected Month</span><span class="card-id">#14</span></div><div class="val-wrap"><span class="unit">Rs.</span><span class="val" id="proj" style="color:#c084fc;">0.00</span></div></div>
    <div class="card border-emerald"><div class="card-top"><span class="card-name">15. CO2 Footprint</span><span class="card-id">#15</span></div><div class="val-wrap"><span class="val" id="co2">0.00</span><span class="unit">kg</span></div></div>
    <div class="card border-emerald"><div class="card-top"><span class="card-name">16. Trees Needed</span><span class="card-id">#16</span></div><div class="val-wrap"><span class="val" id="trees">0.0</span><span class="unit">Trees</span></div></div>
  </div>
  <div class="section-label">Section 4: Peaks & Grid Diagnostics (17 to 20)</div>
  <div class="grid-4">
    <div class="card border-rose"><div class="card-top"><span class="card-name">17. Peak Power</span><span class="card-id">#17</span></div><div class="val-wrap"><span class="val" id="peak_p" style="color:#f87171;">0.0</span><span class="unit">W</span></div></div>
    <div class="card border-rose"><div class="card-top"><span class="card-name">18. Peak Current</span><span class="card-id">#18</span></div><div class="val-wrap"><span class="val" id="peak_i" style="color:#f87171;">0.00</span><span class="unit">A</span></div></div>
    <div class="card border-cyan"><div class="card-top"><span class="card-name">19. Grid Quality</span><span class="card-id">#19</span></div><div class="val-wrap"><span class="val" id="gridQual" style="font-size:16px;color:#38bdf8;">EXCELLENT</span></div></div>
    <div class="card border-emerald"><div class="card-top"><span class="card-name">20. Safety Status</span><span class="card-id">#20</span></div><div class="val-wrap"><span class="val" id="safeStat" style="font-size:16px;color:#34d399;">NORMAL</span></div></div>
  </div>
  <div class="chart-box">
    <div style="font-size:12px;font-weight:700;color:#94a3b8;text-transform:uppercase;margin-bottom:12px;display:flex;justify-content:space-between;flex-wrap:wrap;gap:8px;">
      <span>Real-Time Active Power Dynamic Trend (Watts)</span>
      <span style="color:#38bdf8;">ADC High-Speed Sampling</span>
    </div>
    <canvas id="liveChart" width="1000" height="200"></canvas>
  </div>
</div>
<script>
let powerHistory = new Array(50).fill(0);
let isRelayActive = true;
const canvas = document.getElementById('liveChart');
const ctx = canvas.getContext('2d');
function setGauge(id, val, maxVal) {
  const circle = document.getElementById(id);
  const circ = 2 * Math.PI * 60;
  let ratio = Math.min(Math.max(val / maxVal, 0), 1);
  circle.style.strokeDashoffset = circ - (circ * ratio);
}
function drawChart() {
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  ctx.strokeStyle = 'rgba(255,255,255,0.05)';
  ctx.lineWidth = 1;
  for(let i = 0; i <= 4; i++) {
    let y = (canvas.height / 4) * i;
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(canvas.width, y);
    ctx.stroke();
  }
  let maxP = Math.max(...powerHistory, 100);
  let step = canvas.width / (powerHistory.length - 1);
  ctx.beginPath();
  for(let i = 0; i < powerHistory.length; i++) {
    let x = i * step;
    let y = canvas.height - (powerHistory[i] / maxP * (canvas.height - 30)) - 15;
    if(i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.strokeStyle = '#38bdf8';
  ctx.lineWidth = 3;
  ctx.stroke();
  ctx.lineTo(canvas.width, canvas.height);
  ctx.lineTo(0, canvas.height);
  ctx.closePath();
  let grad = ctx.createLinearGradient(0, 0, 0, canvas.height);
  grad.addColorStop(0, 'rgba(56, 189, 248, 0.35)');
  grad.addColorStop(1, 'rgba(56, 189, 248, 0.0)');
  ctx.fillStyle = grad;
  ctx.fill();
}
function updateData() {
  fetch('/data?t=' + new Date().getTime()).then(r => r.json()).then(d => {
    document.getElementById('v').innerText = d.v.toFixed(1);
    document.getElementById('i').innerText = d.i.toFixed(2);
    document.getElementById('p').innerText = d.p.toFixed(1);
    document.getElementById('va').innerText = d.va.toFixed(1);
    document.getElementById('var').innerText = d.var.toFixed(1);
    document.getElementById('pf').innerText = d.pf.toFixed(2);
    document.getElementById('f').innerText = d.f.toFixed(1);
    document.getElementById('cf').innerText = d.cf.toFixed(2);
    document.getElementById('kwh').innerText = d.kwh.toFixed(4);
    document.getElementById('d_kwh').innerText = d.d_kwh.toFixed(4);
    document.getElementById('hr_rate').innerText = d.hr_rate.toFixed(4);
    document.getElementById('d_cost').innerText = d.d_cost.toFixed(2);
    document.getElementById('bill').innerText = d.bill.toFixed(2);
    document.getElementById('proj').innerText = d.proj.toFixed(2);
    document.getElementById('co2').innerText = d.co2.toFixed(3);
    document.getElementById('trees').innerText = d.trees.toFixed(2);
    document.getElementById('peak_p').innerText = d.peak_p.toFixed(1);
    document.getElementById('peak_i').innerText = d.peak_i.toFixed(2);
    document.getElementById('gridQual').innerText = d.grid;
    document.getElementById('safeStat').innerText = d.status.substring(0, 15);
    document.getElementById('uptime').innerText = d.uptime;
    document.getElementById('g-v').innerText = Math.round(d.v);
    document.getElementById('g-i').innerText = d.i.toFixed(2);
    document.getElementById('g-pf').innerText = d.pf.toFixed(2);
    setGauge('g-v-bar', d.v, 300);
    setGauge('g-i-bar', d.i, 20);
    setGauge('g-pf-bar', d.pf, 1.0);
    isRelayActive = d.relay;
    const rBtn = document.getElementById('relayBtn');
    const rText = document.getElementById('relayText');
    if(isRelayActive) {
      rBtn.className = "btn-relay relay-on";
      rBtn.innerText = "DISCONNECT LOAD";
      rText.innerText = "ONLINE - AC LOAD ACTIVE";
      rText.style.color = "#34d399";
    } else {
      rBtn.className = "btn-relay relay-off";
      rBtn.innerText = "RESTORE LOAD";
      rText.innerText = "OFFLINE - LOAD DISCONNECTED";
      rText.style.color = "#f87171";
    }
    powerHistory.shift();
    powerHistory.push(d.p);
    drawChart();
  }).catch(console.error);
}
function toggleRelay() {
  const target = isRelayActive ? 0 : 1;
  fetch('/relay?state=' + target).then(() => updateData());
}
setInterval(updateData, 1000);
updateData();
drawChart();
</script>
</body>
</html>
)rawliteral";
void performMeasurement() {
  int rawV[ADC_SAMPLES];
  int rawC[ADC_SAMPLES];
  double vMeanSum = 0, cMeanSum = 0;
  unsigned long startSampleTime = micros();
  for(int i = 0; i < ADC_SAMPLES; i++) {
    rawV[i] = analogRead(VOLTAGE_PIN);
    rawC[i] = analogRead(CURRENT_PIN);
    vMeanSum += rawV[i];
    cMeanSum += rawC[i];
    delayMicroseconds(SAMPLE_DELAY_US);
  }
  unsigned long totalTimeUs = micros() - startSampleTime;
  float vMid = vMeanSum / ADC_SAMPLES;
  float cMid = cMeanSum / ADC_SAMPLES;
  double vSqSum = 0.0, cSqSum = 0.0, pInstantSum = 0.0;
  float maxInstantCurrent = 0.0;
  int zeroCrossings = 0;
  float lastSample = 0;
  for(int i = 0; i < ADC_SAMPLES; i++) {
    float vInstant = (float)rawV[i] - vMid;
    float cInstant = (float)rawC[i] - cMid;
    vSqSum += vInstant * vInstant;
    cSqSum += cInstant * cInstant;
    pInstantSum += (vInstant * cInstant);
    if(fabs(cInstant) > maxInstantCurrent) {
      maxInstantCurrent = fabs(cInstant);
    }
    if(lastSample < -5.0 && vInstant >= 5.0) {
      zeroCrossings++;
    }
    lastSample = vInstant;
  }
  voltageRMS = sqrt(vSqSum / ADC_SAMPLES) * VOLT_CAL;
  currentRMS = sqrt(cSqSum / ADC_SAMPLES) * CURR_CAL;
  if(zeroCrossings > 1 && totalTimeUs > 0) {
    frequencyHz = ((float)zeroCrossings / ((float)totalTimeUs / 1000000.0));
  } else {
    frequencyHz = 50.0;
  }
  float peakCurrentSample = maxInstantCurrent * CURR_CAL;
  if(currentRMS > 0.01) {
    crestFactor = peakCurrentSample / currentRMS;
  } else {
    crestFactor = 1.41;
  }
  float rawPower = pInstantSum / ADC_SAMPLES;
  activePower = fabs(rawPower * VOLT_CAL * CURR_CAL);
  apparentPower = voltageRMS * currentRMS;
  if(apparentPower > 0.05) {
    powerFactor = activePower / apparentPower;
    if(powerFactor > 1.0) powerFactor = 1.0;
    reactivePower = sqrt(fabs((apparentPower * apparentPower) - (activePower * activePower)));
  } else {
    powerFactor = 1.0;
    reactivePower = 0.0;
  }
  hourlyRateUnits = activePower / 1000.0;
  dailyCostEst = hourlyRateUnits * 24.0 * unitPrice;
  carbonKg = totalKWh * 0.82;
  treesRequired = carbonKg / 21.77;
  if(activePower > peakPower) peakPower = activePower;
  if(currentRMS > peakCurrent) peakCurrent = currentRMS;
  if(voltageRMS < 20.0) {
    systemStatus = "NO AC INPUT";
    gridQuality = "OFFLINE";
  } else if(activePower > OVERLOAD_LIMIT_WATTS) {
    systemStatus = "OVERLOAD TRIP";
    relayState = false;
    digitalWrite(RELAY_PIN, LOW);
  } else {
    systemStatus = "NORMAL (SAFE)";
    if(powerFactor > 0.88 && voltageRMS >= 210.0 && voltageRMS <= 245.0) gridQuality = "EXCELLENT";
    else if(powerFactor > 0.75) gridQuality = "GOOD";
    else gridQuality = "FAIR (LOSSES)";
  }
}
void updateLCDMultiPage() {
  if (lcd == nullptr) return;
  switch (currentLcdPage) {
    case 0:
      lcd->setCursor(0, 0);
      lcd->printf("[1/5] TRIO TITANS R:%s", relayState ? "ON" : "OF");
      lcd->setCursor(0, 1);
      lcd->printf("V:%5.1fV  I:%4.2fA   ", voltageRMS, currentRMS);
      lcd->setCursor(0, 2);
      lcd->printf("P:%5.1fW  S:%4.0fVA  ", activePower, apparentPower);
      lcd->setCursor(0, 3);
      lcd->printf("Q:%4.0fVAR PF:%4.2f  ", reactivePower, powerFactor);
      break;

    case 1:
      lcd->setCursor(0, 0);
      lcd->print("[2/5] GRID QUALITY  ");
      lcd->setCursor(0, 1);
      lcd->printf("Freq : %4.1f Hz     ", frequencyHz);
      lcd->setCursor(0, 2);
      lcd->printf("Crest: %4.2f         ", crestFactor);
      lcd->setCursor(0, 3);
      lcd->printf("Grid : %-12s ", gridQuality.c_str());
      break;

    case 2:
      lcd->setCursor(0, 0);
      lcd->print("[3/5] ENERGY UNITS  ");
      lcd->setCursor(0, 1);
      lcd->printf("Total: %8.4fkWh   ", totalKWh);
      lcd->setCursor(0, 2);
      lcd->printf("Today: %8.4fkWh   ", dailyKWh);
      lcd->setCursor(0, 3);
      lcd->printf("Rate : %7.4f u/h  ", hourlyRateUnits);
      break;

    case 3:
      lcd->setCursor(0, 0);
      lcd->print("[4/5] BILLING EST   ");
      lcd->setCursor(0, 1);
      lcd->printf("Bill : Rs. %-8.2f ", totalBill);
      lcd->setCursor(0, 2);
      lcd->printf("Day  : Rs. %-8.2f ", dailyCostEst);
      lcd->setCursor(0, 3);
      lcd->printf("Proj : Rs. %-8.2f ", projectedBill);
      break;

    case 4:
      lcd->setCursor(0, 0);
      lcd->print("[5/5] ECO & SAFETY  ");
      lcd->setCursor(0, 1);
      lcd->printf("CO2:%4.2fkg Tr:%3.2f", carbonKg, treesRequired);
      lcd->setCursor(0, 2);
      lcd->printf("Pk:%4.0fW / %4.2fA  ", peakPower, peakCurrent);
      lcd->setCursor(0, 3);
      lcd->printf("Stat:%-15s", systemStatus.substring(0, 15).c_str());
      break;
  }
}

void handleData() {
  unsigned long runSec = (millis() - startTime) / 1000;
  String uptimeStr = String(runSec / 3600) + "h " + String((runSec % 3600) / 60) + "m";

  String json = "{";
  json += "\"v\":" + String(voltageRMS, 1) + ",";
  json += "\"i\":" + String(currentRMS, 2) + ",";
  json += "\"p\":" + String(activePower, 1) + ",";
  json += "\"va\":" + String(apparentPower, 1) + ",";
  json += "\"var\":" + String(reactivePower, 1) + ",";
  json += "\"pf\":" + String(powerFactor, 2) + ",";
  json += "\"f\":" + String(frequencyHz, 1) + ",";
  json += "\"cf\":" + String(crestFactor, 2) + ",";
  json += "\"kwh\":" + String(totalKWh, 4) + ",";
  json += "\"d_kwh\":" + String(dailyKWh, 4) + ",";
  json += "\"hr_rate\":" + String(hourlyRateUnits, 4) + ",";
  json += "\"d_cost\":" + String(dailyCostEst, 2) + ",";
  json += "\"bill\":" + String(totalBill, 2) + ",";
  json += "\"proj\":" + String(projectedBill, 2) + ",";
  json += "\"co2\":" + String(carbonKg, 3) + ",";
  json += "\"trees\":" + String(treesRequired, 2) + ",";
  json += "\"peak_p\":" + String(peakPower, 1) + ",";
  json += "\"peak_i\":" + String(peakCurrent, 2) + ",";
  json += "\"grid\":\"" + gridQuality + "\",";
  json += "\"status\":\"" + systemStatus + "\",";
  json += "\"relay\":" + String(relayState ? "true" : "false") + ",";
  json += "\"uptime\":\"" + uptimeStr + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleRelay() {
  if(server.hasArg("state")) {
    int s = server.arg("state").toInt();
    relayState = (s == 1);
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "INVALID");
  }
}

uint8_t findI2CAddress() {
  uint8_t address = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      if (addr == 0x27 || addr == 0x3F) {
        address = addr;
        break;
      }
    }
  }
  return address;
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  analogReadResolution(12);
  analogSetPinAttenuation(VOLTAGE_PIN, ADC_11db);
  analogSetPinAttenuation(CURRENT_PIN, ADC_11db);

  Wire.begin(SDA_PIN, SCL_PIN, 100000);
  delay(150);

  uint8_t lcdAddress = findI2CAddress();
  if (lcdAddress == 0) lcdAddress = 0x27;

  lcd = new LiquidCrystal_I2C(lcdAddress, LCD_COLUMNS, LCD_ROWS);
  lcd->init();
  lcd->backlight();
  lcd->clear();

  lcd->setCursor(0, 0);
  lcd->print("SMART ENERGY METER  ");
  lcd->setCursor(0, 1);
  lcd->print("TEAM: TRIO TITANS   ");
  lcd->setCursor(0, 2);
  lcd->print("SANJITH,VIKRAM,VISHN");
  lcd->setCursor(0, 3);
  lcd->print("SYSTEM INITIALIZING ");
  delay(2000);

  preferences.begin("energy-20", false);
  totalKWh = preferences.getFloat("energy", 0.0);
  unitPrice = preferences.getFloat("tariff", 8.00);

  if(!isfinite(totalKWh) || totalKWh < 0) totalKWh = 0.0;
  totalBill = totalKWh * unitPrice;

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  WiFi.softAP(AP_SSID);

  dnsServer.start(53, "*", AP_IP);

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", MAIN_PAGE);
  });
  server.on("/data", HTTP_GET, handleData);
  server.on("/relay", HTTP_GET, handleRelay);

  server.onNotFound([]() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();
  lcd->clear();

  startTime = millis();
  lastMeasurement = millis();
  lastEnergyTime = millis();
  lastSaveTime = millis();
  dailyTimer = millis();
  lastLcdPageTime = millis();

  Serial.println("\n============================================================");
  Serial.println("  ESP32 SMART ENERGY METER - TEAM TRIO TITANS READY!        ");
  Serial.println("============================================================");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  unsigned long now = millis();

  if(now - dailyTimer >= 86400000) {
    dailyTimer = now;
    dailyKWh = 0.0;
  }

  if(now - lastMeasurement >= MEASUREMENT_INTERVAL) {
    lastMeasurement = now;
    performMeasurement();

    unsigned long elapsed = now - lastEnergyTime;
    lastEnergyTime = now;

    if(activePower > 0.0 && relayState) {
      float hours = elapsed / 3600000.0;
      float energyDelta = (activePower * hours) / 1000.0;
      totalKWh += energyDelta;
      dailyKWh += energyDelta;
      totalBill = totalKWh * unitPrice;

      float kwLoad = activePower / 1000.0;
      projectedBill = (kwLoad * 24.0 * 30.0) * unitPrice;
    }

    // 20 Parameters Real-time Log Stream
    Serial.println("\n============================================================");
    Serial.println("             SMART METER 20-PARAMETER ANALYSIS LOG          ");
    Serial.println("============================================================");
    Serial.printf(" [01] Voltage (RMS)    : %.1f V\n", voltageRMS);
    Serial.printf(" [02] Current (RMS)    : %.2f A\n", currentRMS);
    Serial.printf(" [03] Active Power     : %.1f W\n", activePower);
    Serial.printf(" [04] Apparent Power   : %.1f VA\n", apparentPower);
    Serial.printf(" [05] Reactive Power   : %.1f VAR\n", reactivePower);
    Serial.printf(" [06] Power Factor     : %.2f\n", powerFactor);
    Serial.printf(" [07] Grid Frequency   : %.1f Hz\n", frequencyHz);
    Serial.printf(" [08] Crest Factor     : %.2f\n", crestFactor);
    Serial.printf(" [09] Total Units      : %.4f kWh\n", totalKWh);
    Serial.printf(" [10] Today's Units    : %.4f kWh\n", dailyKWh);
    Serial.printf(" [11] Run-Rate / Hour  : %.4f units/h\n", hourlyRateUnits);
    Serial.printf(" [12] Est. Daily Cost  : Rs. %.2f\n", dailyCostEst);
    Serial.printf(" [13] Current Bill     : Rs. %.2f\n", totalBill);
    Serial.printf(" [14] Projected Month  : Rs. %.2f\n", projectedBill);
    Serial.printf(" [15] CO2 Footprint    : %.3f kg\n", carbonKg);
    Serial.printf(" [16] Trees Required   : %.2f Trees\n", treesRequired);
    Serial.printf(" [17] Peak Load Power  : %.1f W\n", peakPower);
    Serial.printf(" [18] Peak Current     : %.2f A\n", peakCurrent);
    Serial.printf(" [19] Grid Health      : %s\n", gridQuality.c_str());
    Serial.printf(" [20] Safety Status    : %s\n", systemStatus.c_str());
    Serial.printf(" [*]  AC Relay Status  : %s\n", relayState ? "CONNECTED (ON)" : "DISCONNECTED (OFF)");
    Serial.println("============================================================");

    updateLCDMultiPage();
  }

  if(now - lastLcdPageTime >= LCD_PAGE_INTERVAL) {
    lastLcdPageTime = now;
    currentLcdPage = (currentLcdPage + 1) % 5;
    updateLCDMultiPage();
  }

  if(now - lastSaveTime >= SAVE_INTERVAL) {
    lastSaveTime = now;
    preferences.putFloat("energy", totalKWh);
  }
}