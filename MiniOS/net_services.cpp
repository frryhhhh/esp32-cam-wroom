#include "net_services.h"
#include "config.h"
#include "hw_init.h"
#include "ui_screens.h"

#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <time.h>

static WebServer s_server(80);
static File s_upload_file;
static bool s_ntp_configured = false;

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>MiniOS Control</title>
<style>
  :root { --bg:#000; --card:#12131a; --accent:#667eea; --accent2:#764ba2; --text:#f5f7ff; --muted:#8e95b0; }
  * { box-sizing:border-box; margin:0; padding:0; font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif; }
  body { background:var(--bg); color:var(--text); line-height:1.5; }
  header { position:sticky; top:0; z-index:10; backdrop-filter:blur(16px); background:rgba(0,0,0,0.72); border-bottom:1px solid #1e2230; padding:14px 20px; display:flex; justify-content:space-between; align-items:center; }
  .logo { font-weight:700; font-size:1.25rem; background:linear-gradient(90deg,var(--accent),var(--accent2)); -webkit-background-clip:text; -webkit-text-fill-color:transparent; }
  nav a { color:var(--muted); text-decoration:none; margin-left:16px; font-size:0.9rem; }
  nav a:hover { color:var(--text); }
  .hero { text-align:center; padding:56px 20px 36px; background:radial-gradient(circle at top, rgba(102,126,234,0.22), transparent 65%); }
  .hero h1 { font-size:clamp(2rem,5vw,3.4rem); font-weight:800; letter-spacing:-0.03em; margin-bottom:10px; }
  .hero p { color:var(--muted); max-width:580px; margin:0 auto; }
  .container { max-width:980px; margin:0 auto; padding:16px 20px 64px; display:grid; gap:22px; }
  section { background:var(--card); border:1px solid #1f2436; border-radius:20px; padding:22px; }
  h2 { font-size:1.25rem; margin-bottom:14px; }
  .media-box { width:100%; max-height:360px; border-radius:14px; background:#07080c; object-fit:contain; display:block; border:1px solid #22273b; }
  .btn { background:linear-gradient(135deg,var(--accent),var(--accent2)); color:#fff; border:none; border-radius:12px; padding:10px 16px; font-weight:600; cursor:pointer; }
  .btn:hover { opacity:0.92; }
  .btn-ghost { background:#1b2030; color:var(--text); border:1px solid #2a3148; border-radius:12px; padding:10px 14px; cursor:pointer; }
  .pad { display:grid; grid-template-columns:repeat(3, 84px); gap:10px; justify-content:center; margin-top:10px; }
  .pad button { height:48px; border-radius:12px; background:#1a1f30; color:#fff; border:1px solid #2c344d; font-weight:600; cursor:pointer; }
  .pad button:active { background:var(--accent); }
  .status { font-size:0.85rem; color:var(--muted); margin-top:8px; }
  input[type=text], input[type=password], input[type=file] { width:100%; padding:10px 12px; border-radius:10px; border:1px solid #2a3148; background:#0a0c12; color:#fff; margin-bottom:10px; }
  table { width:100%; border-collapse:collapse; margin-top:10px; font-size:0.9rem; }
  th, td { text-align:left; padding:8px; border-bottom:1px solid #1e2436; }
  canvas { width:100%; height:110px; background:#07080c; border-radius:12px; border:1px solid #22273b; margin-top:10px; display:block; }
</style>
</head>
<body>
<header>
  <div class="logo">MiniOS</div>
  <nav>
    <a href="#cam">Камера</a>
    <a href="#mic">Звук</a>
    <a href="#btns">Кнопки</a>
    <a href="#files">Файлы</a>
    <a href="#wifi">Wi-Fi</a>
    <a href="#ota">OTA</a>
  </nav>
</header>

<div class="hero">
  <h1>MiniOS Studio</h1>
  <p>Управление ESP32-S3: живой поток OV2640, PCM-аудио INMP441, виртуальные кнопки, LittleFS, Wi-Fi и OTA.</p>
</div>

<div class="container">
  <section id="cam">
    <h2>Live-видео (MJPEG /stream)</h2>
    <img id="camImg" class="media-box" src="/stream" alt="MJPEG Stream">
    <div class="status" id="camStatus">Подключение к /stream... (если камера не подключена, сервер вернёт 503)</div>
  </section>

  <section id="mic">
    <h2>Live-звук (PCM 16 кГц /audio + WebAudio)</h2>
    <button class="btn" id="micToggle">Включить звук</button>
    <span class="status" id="micStatus" style="margin-left:10px;">Остановлен</span>
    <canvas id="waveCanvas" width="800" height="110"></canvas>
  </section>

  <section id="btns">
    <h2>Виртуальные кнопки (/cmd?btn=...)</h2>
    <div class="pad">
      <div></div>
      <button onclick="sendBtn('up')">ВВЕРХ</button>
      <div></div>
      <button onclick="sendBtn('left')">ВЛЕВО</button>
      <button onclick="sendBtn('ok')">ОК</button>
      <button onclick="sendBtn('right')">ВПРАВО</button>
      <button onclick="sendBtn('back')">НАЗАД</button>
      <button onclick="sendBtn('down')">ВНИЗ</button>
      <div></div>
    </div>
    <div class="status" id="btnStatus">Готово</div>
  </section>

  <section id="files">
    <h2>Файловый менеджер LittleFS</h2>
    <form id="uploadForm" onsubmit="uploadFile(event)">
      <input type="file" id="fileInput" name="file" required>
      <button class="btn" type="submit">Загрузить в LittleFS (/upload)</button>
    </form>
    <div class="status" id="fsStatus"></div>
    <table>
      <thead><tr><th>Имя файла</th><th>Размер (Б)</th><th>Действие</th></tr></thead>
      <tbody id="fileList"></tbody>
    </table>
  </section>

  <section id="wifi">
    <h2>Настройка Wi-Fi (/setup)</h2>
    <p class="status" style="margin-bottom:10px;">Точка доступа всегда доступна: <b>MiniOS-Setup</b> / пароль <b>ahiralom</b> (192.168.4.1)</p>
    <form onsubmit="saveWifi(event)">
      <input type="text" id="staSsid" placeholder="SSID внешней сети Wi-Fi">
      <input type="password" id="staPass" placeholder="Пароль Wi-Fi">
      <button class="btn" type="submit">Сохранить и подключить</button>
    </form>
    <div class="status" id="wifiStatus"></div>
  </section>

  <section id="ota">
    <h2>OTA-обновление прошивки (/update)</h2>
    <form onsubmit="uploadOta(event)">
      <input type="file" id="otaFile" accept=".bin" required>
      <button class="btn" type="submit">Обновить прошивку (.bin)</button>
    </form>
    <div class="status" id="otaStatus">Выберите скомпилированный файл прошивки (.bin)</div>
  </section>
</div>

<script>
const camImg = document.getElementById('camImg');
const camStatus = document.getElementById('camStatus');
camImg.onerror = () => { camStatus.textContent = 'Поток /stream недоступен (503 — камера не готова или отключена).'; };
camImg.onload = () => { camStatus.textContent = 'Поток активен.'; };

async function sendBtn(name) {
  const res = await fetch('/cmd?btn=' + encodeURIComponent(name));
  document.getElementById('btnStatus').textContent = 'Команда ' + name + ': HTTP ' + res.status;
}

let audioCtx = null, audioRunning = false, audioAbort = null;
const canvas = document.getElementById('waveCanvas');
const ctx2d = canvas.getContext('2d');

function drawWave(samples) {
  ctx2d.fillStyle = '#07080c';
  ctx2d.fillRect(0, 0, canvas.width, canvas.height);
  ctx2d.strokeStyle = '#667eea';
  ctx2d.lineWidth = 2;
  ctx2d.beginPath();
  const step = Math.max(1, Math.floor(samples.length / canvas.width));
  for (let x = 0; x < canvas.width; x++) {
    const idx = Math.min(samples.length - 1, x * step);
    const norm = samples[idx] / 32768;
    const y = (canvas.height / 2) + norm * (canvas.height * 0.45);
    if (x === 0) ctx2d.moveTo(x, y);
    else ctx2d.lineTo(x, y);
  }
  ctx2d.stroke();
}

document.getElementById('micToggle').addEventListener('click', async () => {
  const st = document.getElementById('micStatus');
  if (audioRunning) {
    audioRunning = false;
    if (audioAbort) audioAbort.abort();
    st.textContent = 'Остановлен';
    document.getElementById('micToggle').textContent = 'Включить звук';
    return;
  }
  const resp = await fetch('/audio');
  if (!resp.ok) {
    st.textContent = 'Микрофон недоступен (HTTP ' + resp.status + ')';
    return;
  }
  audioCtx = new (window.AudioContext || window.webkitAudioContext)({ sampleRate: 16000 });
  audioRunning = true;
  document.getElementById('micToggle').textContent = 'Выключить звук';
  st.textContent = 'Воспроизведение PCM 16 кГц...';
  const reader = resp.body.getReader();
  let nextTime = audioCtx.currentTime;
  try {
    while (audioRunning) {
      const { value, done } = await reader.read();
      if (done || !value) break;
      const samples = new Int16Array(value.buffer, value.byteOffset, Math.floor(value.byteLength / 2));
      if (samples.length === 0) continue;
      drawWave(samples);
      const floatBuf = new Float32Array(samples.length);
      for (let i = 0; i < samples.length; i++) floatBuf[i] = samples[i] / 32768;
      const audioBuf = audioCtx.createBuffer(1, floatBuf.length, 16000);
      audioBuf.copyToChannel(floatBuf, 0);
      const src = audioCtx.createBufferSource();
      src.buffer = audioBuf;
      src.connect(audioCtx.destination);
      if (nextTime < audioCtx.currentTime) nextTime = audioCtx.currentTime + 0.02;
      src.start(nextTime);
      nextTime += audioBuf.duration;
    }
  } catch (e) {
    st.textContent = 'Поток звука прерван';
  }
  audioRunning = false;
  document.getElementById('micToggle').textContent = 'Включить звук';
});

async function refreshFiles() {
  const res = await fetch('/api/files');
  const tbody = document.getElementById('fileList');
  tbody.innerHTML = '';
  if (!res.ok) return;
  const list = await res.json();
  for (const f of list) {
    const tr = document.createElement('tr');
    tr.innerHTML = `<td>${f.name}</td><td>${f.size}</td><td><button class="btn-ghost" onclick="delFile('${f.name}')">Удалить</button></td>`;
    tbody.appendChild(tr);
  }
}

async function delFile(name) {
  await fetch('/delete?path=' + encodeURIComponent(name), { method: 'POST' });
  refreshFiles();
}

async function uploadFile(e) {
  e.preventDefault();
  const input = document.getElementById('fileInput');
  if (!input.files.length) return;
  const fd = new FormData();
  fd.append('file', input.files[0]);
  const res = await fetch('/upload', { method: 'POST', body: fd });
  document.getElementById('fsStatus').textContent = await res.text();
  input.value = '';
  refreshFiles();
}

async function saveWifi(e) {
  e.preventDefault();
  const body = new URLSearchParams({
    ssid: document.getElementById('staSsid').value,
    password: document.getElementById('staPass').value
  });
  const res = await fetch('/setup', { method: 'POST', body });
  document.getElementById('wifiStatus').textContent = await res.text();
}

async function uploadOta(e) {
  e.preventDefault();
  const input = document.getElementById('otaFile');
  if (!input.files.length) return;
  const fd = new FormData();
  fd.append('update', input.files[0]);
  document.getElementById('otaStatus').textContent = 'Загрузка прошивки...';
  const res = await fetch('/update', { method: 'POST', body: fd });
  document.getElementById('otaStatus').textContent = await res.text();
}

refreshFiles();
</script>
</body>
</html>
)HTML";

void net_load_configs(ThemeConfig& theme, WiFiCreds& wifiCfg) {
  theme = {0x0B0F19, 0x667EEA, 0x161E31, 0xF8FAFC, "Midnight Indigo"};
  memset(&wifiCfg, 0, sizeof(wifiCfg));
  wifiCfg.enabled = false;

  if (!g_fs_ready) return;

  if (LittleFS.exists(THEME_CFG_PATH)) {
    File f = LittleFS.open(THEME_CFG_PATH, "r");
    if (f) {
      JsonDocument doc;
      if (!deserializeJson(doc, f)) {
        theme.bgHex = doc["bgHex"] | theme.bgHex;
        theme.accentHex = doc["accentHex"] | theme.accentHex;
        theme.cardHex = doc["cardHex"] | theme.cardHex;
        theme.textHex = doc["textHex"] | theme.textHex;
        strlcpy(theme.themeName, doc["themeName"] | theme.themeName, sizeof(theme.themeName));
      }
      f.close();
    }
  }

  if (LittleFS.exists(WIFI_CFG_PATH)) {
    File f = LittleFS.open(WIFI_CFG_PATH, "r");
    if (f) {
      JsonDocument doc;
      if (!deserializeJson(doc, f)) {
        strlcpy(wifiCfg.ssid, doc["ssid"] | "", sizeof(wifiCfg.ssid));
        strlcpy(wifiCfg.password, doc["password"] | "", sizeof(wifiCfg.password));
        wifiCfg.enabled = (strlen(wifiCfg.ssid) > 0);
      }
      f.close();
    }
  }
}

void net_save_theme(const ThemeConfig& theme) {
  if (!g_fs_ready) return;
  File f = LittleFS.open(THEME_CFG_PATH, "w");
  if (!f) return;
  JsonDocument doc;
  doc["bgHex"] = theme.bgHex;
  doc["accentHex"] = theme.accentHex;
  doc["cardHex"] = theme.cardHex;
  doc["textHex"] = theme.textHex;
  doc["themeName"] = theme.themeName;
  serializeJson(doc, f);
  f.close();
}

void net_save_wifi(const WiFiCreds& wifiCfg) {
  if (!g_fs_ready) return;
  File f = LittleFS.open(WIFI_CFG_PATH, "w");
  if (!f) return;
  JsonDocument doc;
  doc["ssid"] = wifiCfg.ssid;
  doc["password"] = wifiCfg.password;
  serializeJson(doc, f);
  f.close();
}

static void handle_root() {
  s_server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

static void handle_stream() {
#if !defined(ENABLE_CAMERA)
  s_server.send(503, "text/plain", "Camera disabled");
  return;
#else
  if (!g_cam_ready) {
    s_server.send(503, "text/plain", "Camera not ready");
    return;
  }

  WiFiClient client = s_server.client();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Cache-Control: no-cache");
  client.println("Connection: close");
  client.println();

  while (client.connected()) {
    if (!g_cam_ready) {
      break;
    }
    uint8_t* jpg = nullptr;
    size_t jpg_len = 0;
    if (!hw_capture_jpeg(&jpg, &jpg_len)) {
      break;
    }

    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", (unsigned)jpg_len);
    size_t written = client.write(jpg, jpg_len);
    client.print("\r\n");
    hw_release_jpeg(jpg);

    if (written != jpg_len) {
      break;
    }
    delay(35);
    yield();
  }
  client.stop();
#endif
}

static void handle_audio() {
#if !defined(ENABLE_MIC_I2S)
  s_server.send(503, "text/plain", "Microphone disabled");
  return;
#else
  if (!g_mic_ready) {
    s_server.send(503, "text/plain", "Microphone not ready");
    return;
  }

  WiFiClient client = s_server.client();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/octet-stream");
  client.println("X-Sample-Rate: 16000");
  client.println("Cache-Control: no-cache");
  client.println("Connection: close");
  client.println();

  static constexpr size_t CHUNK_SAMPLES = 512;
  int16_t pcm[CHUNK_SAMPLES];

  while (client.connected() && g_mic_ready) {
    size_t got = hw_read_mic_pcm16(pcm, CHUNK_SAMPLES);
    if (got == 0) {
      break;
    }
    size_t bytes = got * sizeof(int16_t);
    size_t sent = client.write((const uint8_t*)pcm, bytes);
    if (sent != bytes) {
      break;
    }
    yield();
  }
  client.stop();
#endif
}

static void handle_cmd() {
  String b = s_server.arg("btn");
  b.toLowerCase();
  ButtonId id = BTN_NONE;
  if (b == "up") id = BTN_UP;
  else if (b == "down") id = BTN_DOWN;
  else if (b == "left") id = BTN_LEFT;
  else if (b == "right") id = BTN_RIGHT;
  else if (b == "ok") id = BTN_OK;
  else if (b == "back") id = BTN_BACK;

  if (id == BTN_NONE) {
    s_server.send(400, "text/plain", "Unknown button");
    return;
  }

  hw_inject_virtual_button(id);
  s_server.send(200, "text/plain", "OK");
}

static void handle_api_files() {
  if (!g_fs_ready) {
    s_server.send(503, "application/json", "[]");
    return;
  }

  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  File root = LittleFS.open("/");
  if (root && root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        JsonObject obj = arr.add<JsonObject>();
        String n = String(file.name());
        if (!n.startsWith("/")) n = "/" + n;
        obj["name"] = n;
        obj["size"] = (uint32_t)file.size();
      }
      file = root.openNextFile();
    }
  }
  String out;
  serializeJson(doc, out);
  s_server.send(200, "application/json", out);
}

static void handle_delete() {
  if (!g_fs_ready) {
    s_server.send(503, "text/plain", "FS unavailable");
    return;
  }
  String path = s_server.arg("path");
  if (path.length() == 0) {
    s_server.send(400, "text/plain", "Missing path");
    return;
  }
  if (!path.startsWith("/")) path = "/" + path;
  if (LittleFS.remove(path)) {
    s_server.send(200, "text/plain", "Deleted");
  } else {
    s_server.send(404, "text/plain", "Not found");
  }
}

static void handle_upload_post() {
  HTTPUpload& up = s_server.upload();
  if (up.status == UPLOAD_FILE_START) {
    if (!g_fs_ready) return;
    String p = up.filename;
    if (!p.startsWith("/")) p = "/" + p;
    s_upload_file = LittleFS.open(p, "w");
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (s_upload_file) {
      s_upload_file.write(up.buf, up.currentSize);
    }
  } else if (up.status == UPLOAD_FILE_END || up.status == UPLOAD_FILE_ABORTED) {
    if (s_upload_file) {
      s_upload_file.close();
    }
  }
}

static void handle_upload_done() {
  if (!g_fs_ready) {
    s_server.send(503, "text/plain", "FS unavailable");
    return;
  }
  s_server.send(200, "text/plain", "Uploaded to LittleFS");
}

static void handle_setup() {
  String ssid = s_server.arg("ssid");
  String pass = s_server.arg("password");
  ssid.trim();
  if (ssid.length() == 0 || ssid.length() >= 33 || pass.length() >= 65) {
    s_server.send(400, "text/plain", "Invalid SSID or password length");
    return;
  }

  WiFiCreds cfg = {};
  strlcpy(cfg.ssid, ssid.c_str(), sizeof(cfg.ssid));
  strlcpy(cfg.password, pass.c_str(), sizeof(cfg.password));
  cfg.enabled = true;
  net_save_wifi(cfg);

  // Точка доступа остаётся неизменной (MiniOS-Setup / ahiralom / 192.168.4.1)
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(AP_LOCAL_IP, AP_GATEWAY, AP_SUBNET);
  WiFi.softAP(AP_SSID, AP_PASS);
  WiFi.begin(cfg.ssid, cfg.password);

  s_server.send(200, "text/plain", "Wi-Fi credentials saved. AP remains MiniOS-Setup.");
}

static void handle_update_post() {
  HTTPUpload& up = s_server.upload();
  if (up.status == UPLOAD_FILE_START) {
    Serial.printf("[OTA] Start: %s\n", up.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (Update.write(up.buf, up.currentSize) != up.currentSize) {
      Update.printError(Serial);
    }
  } else if (up.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("[OTA] Success: %u bytes. Rebooting...\n", up.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

static void handle_update_done() {
  if (Update.hasError()) {
    s_server.send(500, "text/plain", "OTA failed");
  } else {
    s_server.send(200, "text/plain", "OTA success! Rebooting...");
    delay(500);
    ESP.restart();
  }
}

void net_init_wifi_and_server(const WiFiCreds& wifiCfg) {
  WiFi.setSleep(false);
  WiFi.mode(wifiCfg.enabled ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAPConfig(AP_LOCAL_IP, AP_GATEWAY, AP_SUBNET);
  WiFi.softAP(AP_SSID, AP_PASS);

  if (wifiCfg.enabled && strlen(wifiCfg.ssid) > 0) {
    WiFi.begin(wifiCfg.ssid, wifiCfg.password);
    Serial.printf("[WIFI] AP '%s' started (192.168.4.1) + STA connecting to '%s'\n", AP_SSID, wifiCfg.ssid);
  } else {
    Serial.printf("[WIFI] AP '%s' started (192.168.4.1)\n", AP_SSID);
  }

  s_server.on("/", HTTP_GET, handle_root);
  s_server.on("/stream", HTTP_GET, handle_stream);
  s_server.on("/audio", HTTP_GET, handle_audio);
  s_server.on("/cmd", HTTP_GET, handle_cmd);
  s_server.on("/api/files", HTTP_GET, handle_api_files);
  s_server.on("/delete", HTTP_POST, handle_delete);
  s_server.on("/upload", HTTP_POST, handle_upload_done, handle_upload_post);
  s_server.on("/setup", HTTP_POST, handle_setup);
  s_server.on("/update", HTTP_POST, handle_update_done, handle_update_post);
  s_server.begin();
  Serial.println("[HTTP] WebServer listening on port 80.");
}

void net_loop() {
  s_server.handleClient();
}

bool net_is_sta_connected() {
  return (WiFi.status() == WL_CONNECTED);
}

void net_update_ntp_and_clock(char* outTimeStr, size_t timeLen, char* outDateStr, size_t dateLen, bool* outNtpSynced) {
  if (net_is_sta_connected() && !s_ntp_configured) {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2);
    s_ntp_configured = true;
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 15)) {
    if (outNtpSynced) *outNtpSynced = (timeinfo.tm_year >= (2025 - 1900));
    strftime(outTimeStr, timeLen, "%H:%M", &timeinfo);
    strftime(outDateStr, dateLen, "%a, %b %d", &timeinfo);
  } else {
    if (outNtpSynced) *outNtpSynced = false;
    uint32_t sec = millis() / 1000;
    uint32_t hh = (sec / 3600) % 24;
    uint32_t mm = (sec / 60) % 60;
    snprintf(outTimeStr, timeLen, "%02lu:%02lu", (unsigned long)hh, (unsigned long)mm);
    snprintf(outDateStr, dateLen, "AP 192.168.4.1");
  }
}
