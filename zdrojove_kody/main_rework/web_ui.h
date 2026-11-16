#pragma once
#include <Arduino.h>

// Single-page web app. Fully self-contained (no CDN) so it also works on the lamp's own hotspot.
static const char INDEX_HTML[] PROGMEM = R"html(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#0f1115">
<title>Mood Lamp</title>
<style>
:root{--bg:#0f1115;--card:#181b22;--line:#262a33;--text:#e8eaf0;--muted:#8b91a1;--glow:#ff8c3c}
*{box-sizing:border-box}
body{margin:0;font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;background:var(--bg);color:var(--text);min-height:100vh;-webkit-tap-highlight-color:transparent}
.glow{position:fixed;left:-30%;right:-30%;top:-35vh;height:80vh;background:radial-gradient(closest-side,var(--glow),transparent);opacity:.3;filter:blur(30px);pointer-events:none;transition:opacity .6s;z-index:0}
main{position:relative;z-index:1;max-width:480px;margin:0 auto;padding:20px 16px 40px}
header{display:flex;align-items:center;justify-content:space-between;margin-bottom:18px}
h1{font-size:22px;margin:0;font-weight:650}
.net{display:flex;align-items:center;gap:8px;font-size:13px;color:var(--muted)}
.dot{width:8px;height:8px;border-radius:50%;background:#666;transition:.3s}
.dot.ok{background:#3ddc84;box-shadow:0 0 8px #3ddc84}
.dot.warn{background:#b06bff;box-shadow:0 0 8px #b06bff}
.dot.bad{background:#ff5d5d}
.card{background:var(--card);border:1px solid var(--line);border-radius:18px;padding:18px;margin-bottom:14px;transition:opacity .3s}
.power{display:flex;align-items:center;gap:18px}
#power{width:76px;height:76px;flex:none;border-radius:50%;border:none;background:#232733;color:var(--muted);cursor:pointer;display:grid;place-items:center;transition:.35s}
#power svg{width:34px;height:34px}
#power.on{background:var(--glow);color:#111;box-shadow:0 0 34px var(--glow)}
.plabel b{display:block;font-size:19px}
.plabel span{color:var(--muted);font-size:13px}
h2{font-size:12px;text-transform:uppercase;letter-spacing:1.2px;color:var(--muted);margin:0 0 12px;font-weight:600}
.row{display:flex;justify-content:space-between;font-size:14px;color:var(--muted);margin-bottom:6px}
.row b{color:var(--text);font-weight:600}
input[type=range]{-webkit-appearance:none;appearance:none;width:100%;height:32px;background:transparent;margin:0 0 6px}
input[type=range]::-webkit-slider-runnable-track{height:8px;border-radius:4px;background:linear-gradient(90deg,#2a2f3a,var(--glow))}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:26px;height:26px;border-radius:50%;background:#fff;margin-top:-9px;box-shadow:0 2px 6px rgba(0,0,0,.5)}
input[type=range]::-moz-range-track{height:8px;border-radius:4px;background:linear-gradient(90deg,#2a2f3a,var(--glow))}
input[type=range]::-moz-range-thumb{width:26px;height:26px;border:none;border-radius:50%;background:#fff}
.colors{display:flex;gap:10px;flex-wrap:wrap}
.sw,.picker{width:40px;height:40px;border-radius:50%;border:2px solid transparent;cursor:pointer;padding:0;position:relative;overflow:hidden}
.sw.sel,.picker.sel{border-color:#fff;box-shadow:0 0 0 2px var(--bg) inset}
.picker{background:conic-gradient(red,yellow,lime,cyan,blue,magenta,red)}
.picker input{position:absolute;inset:-10px;width:64px;height:64px;opacity:0;cursor:pointer;border:none}
.fx{display:grid;grid-template-columns:repeat(auto-fill,minmax(130px,1fr));gap:8px}
.fx button{background:#20242d;border:1px solid var(--line);color:var(--text);border-radius:12px;padding:12px;font-size:14px;cursor:pointer;text-align:left;transition:.2s;font-family:inherit}
.fx button small{display:block;color:var(--muted);font-size:11px;margin-top:3px}
.fx button.sel{border-color:var(--glow);background:#262019;box-shadow:inset 0 0 0 1px var(--glow)}
.dim{opacity:.4}
footer{color:var(--muted);font-size:12px;text-align:center;margin-top:18px;line-height:1.7;white-space:pre-line}
</style>
</head>
<body>
<div class="glow" id="glow"></div>
<main>
  <header>
    <h1>Mood Lamp</h1>
    <div class="net"><span class="dot" id="dot"></span><span id="netTxt">Connecting…</span></div>
  </header>

  <section class="card power">
    <button id="power" aria-label="Power">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"><path d="M12 3v9"/><path d="M6.3 6.8a8 8 0 1 0 11.4 0"/></svg>
    </button>
    <div class="plabel"><b id="powTxt">—</b><span id="fxTxt"></span></div>
  </section>

  <section class="card" id="colorCard">
    <h2>Color</h2>
    <div class="colors" id="colors">
      <label class="picker" id="pickerWrap" title="Custom color"><input type="color" id="color"></label>
    </div>
  </section>

  <section class="card">
    <div class="row">Brightness <b id="briVal"></b></div>
    <input type="range" id="bri" min="1" max="100">
    <div class="row">Speed <b id="spdVal"></b></div>
    <input type="range" id="spd" min="1" max="100">
  </section>

  <section class="card">
    <h2>Effects</h2>
    <div class="fx" id="fx"></div>
  </section>

  <footer id="foot"></footer>
</main>
<script>
const $ = id => document.getElementById(id);
const PRESETS = ['#ff8c3c','#ffd8a8','#ffffff','#ff2a2a','#ff3d9a','#9b4dff','#2f6bff','#00d2ff','#22e07a','#ffe14d'];
let st = null, effects = [], pending = {}, timer = null, lastSent = 0, lastInput = 0;

function render(s) {
  st = s;
  const fx = effects.find(f => f.id === s.effect);
  const glow = !s.on ? '#2a2d36' : (fx && !fx.usesColor ? '#8a5cff' : s.color);
  document.documentElement.style.setProperty('--glow', glow);
  $('glow').style.opacity = s.on ? (0.12 + 0.33 * s.brightness / 100) : 0.04;
  $('power').classList.toggle('on', s.on);
  $('powTxt').textContent = s.on ? 'On' : 'Off';
  $('fxTxt').textContent = fx ? fx.name : '';
  $('color').value = s.color;
  let preset = false;
  document.querySelectorAll('.sw').forEach(b => { const m = b.dataset.c === s.color; preset ||= m; b.classList.toggle('sel', m); });
  $('pickerWrap').classList.toggle('sel', !preset);
  $('colorCard').classList.toggle('dim', !!fx && !fx.usesColor);
  $('bri').value = s.brightness; $('briVal').textContent = s.brightness + ' %';
  $('spd').value = s.speed; $('spdVal').textContent = s.speed;
  document.querySelectorAll('#fx button').forEach(b => b.classList.toggle('sel', +b.dataset.id === s.effect));
  renderNet(s.wifi);
}

function renderNet(w, offline) {
  const d = $('dot');
  if (offline) { d.className = 'dot bad'; $('netTxt').textContent = 'Lamp unreachable'; return; }
  if (!w) return;
  if (w.state === 'connected') { d.className = 'dot ok'; $('netTxt').textContent = w.ssid + ' · ' + w.rssi + ' dBm'; }
  else if (w.ap) { d.className = 'dot warn'; $('netTxt').textContent = 'Hotspot mode'; }
  else { d.className = 'dot bad'; $('netTxt').textContent = 'Reconnecting…'; }
  const lines = [];
  if (w.ip) lines.push('http://' + w.host + '  ·  ' + w.ip);
  if (w.ap) lines.push('Hotspot ' + w.apSsid + '  ·  ' + w.apIp);
  $('foot').textContent = lines.join('\n');
}

// Optimistic local update + throttled send (max ~10 req/s, last value always sent).
function change(patch) {
  if (!st) return;
  Object.assign(st, patch);
  render(st);
  Object.assign(pending, patch);
  lastInput = Date.now();
  if (!timer) timer = setTimeout(flush, Math.max(0, 100 - (Date.now() - lastSent)));
}

async function flush() {
  timer = null;
  const body = pending; pending = {};
  lastSent = Date.now();
  try {
    const r = await fetch('/api/state', {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify(body)});
    if (!r.ok) throw new Error(r.status);
    const s = await r.json();
    if (!timer && Date.now() - lastInput > 400) render(s);
  } catch (e) {
    renderNet(null, true);
    pending = Object.assign(body, pending);
    if (!timer) timer = setTimeout(flush, 1500);
  }
}

async function poll(force) {
  if (!force && (timer || Date.now() - lastInput < 1500)) return;
  try {
    const r = await fetch('/api/state', {cache: 'no-store'});
    render(await r.json());
  } catch (e) { renderNet(null, true); }
}

async function loadEffects() {
  try { effects = await (await fetch('/api/effects')).json(); } catch (e) { return; }
  const box = $('fx'); box.textContent = '';
  effects.forEach(f => {
    const b = document.createElement('button');
    b.dataset.id = f.id;
    b.textContent = f.name;
    const sm = document.createElement('small');
    sm.textContent = f.usesColor ? 'uses your color' : 'own colors';
    b.appendChild(sm);
    b.onclick = () => change({effect: f.id, on: true});
    box.appendChild(b);
  });
}

PRESETS.forEach(c => {
  const b = document.createElement('button');
  b.className = 'sw'; b.style.background = c; b.dataset.c = c; b.title = c;
  b.onclick = () => change({color: c, on: true});
  $('colors').insertBefore(b, $('pickerWrap'));
});
$('power').onclick = () => st && change({on: !st.on});
$('color').oninput = e => change({color: e.target.value, on: true});
$('bri').oninput = e => change({brightness: +e.target.value});
$('spd').oninput = e => change({speed: +e.target.value});

(async () => {
  await loadEffects();
  await poll(true);
  setInterval(() => { if (!effects.length) loadEffects(); poll(false); }, 5000);
})();
</script>
</body>
</html>
)html";
