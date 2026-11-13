// Web server nhận dữ liệu từ trạm đo lũ ESP32 và hiển thị dashboard.
// Chạy: node server.js   (không cần npm install)
const http = require('http');
const fs = require('fs');
const path = require('path');
const { HORIZONS_S, forecastLatest } = require('./forecast');

const PORT = Number(process.env.PORT) || 3000;
const DATA_FILE = path.join(__dirname, 'data.jsonl');   // mỗi dòng một bản tin
const MAX_HISTORY = 2000;                               // số bản tin tối đa /api/history trả về
const KEEP_DAYS = 7;                                    // giữ 7 ngày gần nhất trong bộ nhớ cho bảng theo ngày
const LEVELS = ['normal', 'yellow', 'red'];
const NUM_FIELDS = ['h_nuoc_cm', 'd_loc_cm', 'h_tram_cm', 'flow_lpm', 'total_l', 'soil_pct'];

// Ngày theo giờ máy chạy server, dạng YYYY-MM-DD
const pad = n => String(n).padStart(2, '0');
const dayKey = ms => { const d = new Date(ms); return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())}`; };
const keepFrom = () => { const d = new Date(); d.setHours(0, 0, 0, 0); return d.setDate(d.getDate() - (KEEP_DAYS - 1)); };

// Nạp lại bản tin của 7 ngày gần nhất khi khởi động (data.jsonl vẫn giữ toàn bộ)
let history = [];
if (fs.existsSync(DATA_FILE)) {
  const from = keepFrom();
  history = fs.readFileSync(DATA_FILE, 'utf8').split('\n').filter(Boolean)
    .map(line => { try { return JSON.parse(line); } catch { return null; } })
    .filter(r => r && Date.parse(r.received_at) >= from);
}

// Tóm tắt từng ngày trong 7 ngày gần nhất, ngày mới nhất trước; ngày không có bản tin có count = 0
function daySummaries() {
  const days = new Map();
  for (let i = 0; i < KEEP_DAYS; i++) {
    const d = new Date(); d.setDate(d.getDate() - i);
    days.set(dayKey(d), { date: dayKey(d), count: 0, alerts: 0, max_level: 'normal', h_min: null, h_max: null, h_sum: 0, flow_max: 0, litres: 0, soil_sum: 0, prevTotal: null });
  }
  for (const r of history) {
    const s = days.get(dayKey(Date.parse(r.received_at)));
    if (!s) continue;
    s.count++;
    if (r.event === 'alert' && r.level !== 'normal') s.alerts++;
    if (LEVELS.indexOf(r.level) > LEVELS.indexOf(s.max_level)) s.max_level = r.level;
    s.h_min = s.h_min === null ? r.h_nuoc_cm : Math.min(s.h_min, r.h_nuoc_cm);
    s.h_max = s.h_max === null ? r.h_nuoc_cm : Math.max(s.h_max, r.h_nuoc_cm);
    s.h_sum += r.h_nuoc_cm; s.soil_sum += r.soil_pct;
    s.flow_max = Math.max(s.flow_max, r.flow_lpm);
    // total_l về 0 khi ESP32 khởi động lại: khi đó cộng luôn giá trị mới thay vì lấy hiệu
    if (s.prevTotal !== null) s.litres += r.total_l >= s.prevTotal ? r.total_l - s.prevTotal : r.total_l;
    s.prevTotal = r.total_l;
  }
  return [...days.values()].map(({ h_sum, soil_sum, prevTotal, ...s }) => ({
    ...s, h_avg: s.count ? h_sum / s.count : null, soil_avg: s.count ? soil_sum / s.count : null,
  }));
}

// Mô hình dự báo mực nước (tạo bằng train.js). Không có file thì dashboard chỉ không hiện dự báo.
const MODEL_FILE = path.join(__dirname, 'model.json');
let model = null;
try { model = JSON.parse(fs.readFileSync(MODEL_FILE, 'utf8')); }
catch { console.log('Chua co model.json, tat du bao. Chay: node generate-data.js && node train.js'); }

// Thời tiết từ OpenWeatherMap. API key để trong weather.json (không đưa lên git):
//   { "apiKey": "...", "city": "Hanoi,VN" }
// Không có file thì dashboard ẩn phần thời tiết.
const WEATHER_FILE = path.join(__dirname, 'weather.json');
const WEATHER_TTL_MS = 10 * 60 * 1000;   // gọi API tối đa 10 phút/lần, dashboard hỏi mỗi phút
let weatherCfg = null;
try { weatherCfg = JSON.parse(fs.readFileSync(WEATHER_FILE, 'utf8')); }
catch { console.log('Chua co weather.json, tat phan thoi tiet'); }
let weatherCache = { at: 0, data: null }, weatherPending = null;

async function fetchWeather() {
  const q = `q=${encodeURIComponent(weatherCfg.city)}&units=metric&lang=vi&appid=${encodeURIComponent(weatherCfg.apiKey)}`;
  const get = p => fetch(`https://api.openweathermap.org/data/2.5/${p}?${q}`, { signal: AbortSignal.timeout(8000) })
    .then(r => r.ok ? r.json() : Promise.reject(new Error(`OpenWeatherMap tra ${r.status}`)));
  const [cur, fc] = await Promise.all([get('weather'), get('forecast')]);
  // 8 mốc 3 giờ = 24 giờ tới
  const next = fc.list.slice(0, 8).map(f => ({
    time: f.dt * 1000, temp: f.main.temp, rain_mm: (f.rain && f.rain['3h']) || 0, pop: f.pop,
    icon: f.weather[0].icon, desc: f.weather[0].description,
  }));
  return {
    city: cur.name, temp: cur.main.temp, feels: cur.main.feels_like, humidity: cur.main.humidity, wind: cur.wind.speed,
    desc: cur.weather[0].description, icon: cur.weather[0].icon, rain_1h: (cur.rain && cur.rain['1h']) || 0,
    rain_24h: next.reduce((s, f) => s + f.rain_mm, 0), next, updated: Date.now(),
  };
}

async function getWeather() {
  if (weatherCache.data && Date.now() - weatherCache.at < WEATHER_TTL_MS) return weatherCache.data;
  weatherPending = weatherPending || fetchWeather()
    .then(data => { weatherCache = { at: Date.now(), data }; return data; })
    .finally(() => { weatherPending = null; });
  return weatherPending;
}

// Kiểm tra body theo đúng hợp đồng trong README mục 4.1
function validate(d) {
  if (!d || typeof d !== 'object') return 'body phai la JSON object';
  if (d.event !== 'alert' && d.event !== 'report') return 'event phai la "alert" hoac "report"';
  if (!LEVELS.includes(d.level)) return 'level phai la normal/yellow/red';
  for (const f of NUM_FIELDS) if (typeof d[f] !== 'number') return `${f} phai la so`;
  return null;
}

function sendJson(res, code, obj) {
  res.writeHead(code, { 'Content-Type': 'application/json; charset=utf-8' });
  res.end(JSON.stringify(obj));
}

const server = http.createServer((req, res) => {
  const url = new URL(req.url, 'http://x');

  // ESP32 gửi dữ liệu lên đây
  if (req.method === 'POST' && url.pathname === '/flood') {
    let body = '';
    req.on('data', chunk => {
      body += chunk;
      if (body.length > 10000) req.destroy();   // chặn body quá lớn
    });
    req.on('end', () => {
      let d;
      try { d = JSON.parse(body); } catch { return sendJson(res, 400, { error: 'JSON khong hop le' }); }
      const err = validate(d);
      if (err) return sendJson(res, 400, { error: err });

      const rec = { received_at: new Date().toISOString() };   // ESP32 không có đồng hồ, server gắn giờ
      for (const k of ['event', 'level', ...NUM_FIELDS]) rec[k] = d[k];
      history.push(rec);
      const from = keepFrom();
      while (history.length && Date.parse(history[0].received_at) < from) history.shift();
      fs.appendFile(DATA_FILE, JSON.stringify(rec) + '\n', e => e && console.error('Loi ghi file:', e));
      console.log(rec.received_at, rec.event, rec.level, `H=${rec.h_nuoc_cm}cm`);
      sendJson(res, 200, { ok: true });
    });
    return;
  }

  // Dashboard lấy dữ liệu
  if (req.method === 'GET' && url.pathname === '/api/history') {
    const limit = Math.min(parseInt(url.searchParams.get('limit')) || 300, MAX_HISTORY);
    return sendJson(res, 200, history.slice(-limit));
  }

  // Mực nước dự báo; h_cm = null khi chưa có model hoặc chưa đủ 5 phút dữ liệu liên tục
  if (req.method === 'GET' && url.pathname === '/api/forecast') {
    // Chỉ cần 5 phút cuối; 200 bản tin là đủ dư
    return sendJson(res, 200, { horizons_min: HORIZONS_S.map(s => s / 60), h_cm: forecastLatest(model, history.slice(-200)) });
  }

  // Thời tiết hiện tại và lượng mưa 24 giờ tới; lỗi mạng thì trả bản cũ trong bộ nhớ nếu có
  if (req.method === 'GET' && url.pathname === '/api/weather') {
    if (!weatherCfg) return sendJson(res, 200, { enabled: false });
    getWeather().then(
      data => sendJson(res, 200, { enabled: true, ...data }),
      e => {
        console.error('Loi lay thoi tiet:', e.message);
        if (weatherCache.data) sendJson(res, 200, { enabled: true, stale: true, ...weatherCache.data });
        else sendJson(res, 502, { enabled: true, error: 'Khong lay duoc thoi tiet' });
      });
    return;
  }

  // Tóm tắt 7 ngày gần nhất
  if (req.method === 'GET' && url.pathname === '/api/days') {
    return sendJson(res, 200, daySummaries());
  }

  // Toàn bộ bản tin của một ngày, ?date=YYYY-MM-DD
  if (req.method === 'GET' && url.pathname === '/api/day') {
    const date = url.searchParams.get('date') || '';
    if (!/^\d{4}-\d{2}-\d{2}$/.test(date)) return sendJson(res, 400, { error: 'date phai co dang YYYY-MM-DD' });
    return sendJson(res, 200, history.filter(r => dayKey(Date.parse(r.received_at)) === date));
  }

  if (req.method === 'GET' && (url.pathname === '/' || url.pathname === '/index.html')) {
    res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
    return fs.createReadStream(path.join(__dirname, 'index.html')).pipe(res);
  }

  sendJson(res, 404, { error: 'khong tim thay' });
});

// Lắng nghe 0.0.0.0 để ESP32 trong mạng LAN gửi tới được
server.listen(PORT, '0.0.0.0', () => {
  console.log(`Dang nghe cong ${PORT}. Dashboard: http://localhost:${PORT}`);
  console.log(`Da nap ${history.length} ban tin cua ${KEEP_DAYS} ngay gan nhat tu ${path.basename(DATA_FILE)}`);
});
