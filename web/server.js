// Web server nhận dữ liệu từ trạm đo lũ ESP32 và hiển thị dashboard.
// Chạy: node server.js   (không cần npm install)
const http = require('http');
const fs = require('fs');
const path = require('path');

const PORT = 3000;
const DATA_FILE = path.join(__dirname, 'data.jsonl');   // mỗi dòng một bản tin
const MAX_HISTORY = 2000;                               // số bản tin giữ trong bộ nhớ
const LEVELS = ['normal', 'yellow', 'red'];
const NUM_FIELDS = ['h_nuoc_cm', 'd_loc_cm', 'h_tram_cm', 'flow_lpm', 'total_l', 'soil_pct'];

// Nạp lại lịch sử khi khởi động
let history = [];
if (fs.existsSync(DATA_FILE)) {
  history = fs.readFileSync(DATA_FILE, 'utf8').split('\n').filter(Boolean)
    .map(line => { try { return JSON.parse(line); } catch { return null; } })
    .filter(Boolean).slice(-MAX_HISTORY);
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
      if (history.length > MAX_HISTORY) history.shift();
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

  if (req.method === 'GET' && (url.pathname === '/' || url.pathname === '/index.html')) {
    res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
    return fs.createReadStream(path.join(__dirname, 'index.html')).pipe(res);
  }

  sendJson(res, 404, { error: 'khong tim thay' });
});

// Lắng nghe 0.0.0.0 để ESP32 trong mạng LAN gửi tới được
server.listen(PORT, '0.0.0.0', () => {
  console.log(`Dang nghe cong ${PORT}. Dashboard: http://localhost:${PORT}`);
  console.log(`Da nap ${history.length} ban tin cu tu ${path.basename(DATA_FILE)}`);
});
