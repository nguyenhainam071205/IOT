// Sinh dữ liệu giả lập của trạm đo lũ, cùng định dạng với data.jsonl mà server ghi.
// Mô phỏng: mưa -> đất ẩm dần -> nước chảy tràn qua ống (YF-S201) -> mực nước trong bể dâng.
// Chạy: node generate-data.js [số ngày=30] [seed=1]   -> ghi ra synthetic.jsonl
const fs = require('fs');
const path = require('path');

const DAYS = Number(process.argv[2]) || 30;
let seed = Number(process.argv[3]) || 1;

// Cùng hằng số với firmware/config.h
const H_TRAM_CM = 30, YELLOW_CM = 4, RED_CM = 8, HYST_CM = 0.5;
const DT_S = 5;   // ESP32 báo cáo 5 giây/lần

// Số ngẫu nhiên có seed (mulberry32) để chạy lại ra đúng bộ dữ liệu cũ
function rand() {
  seed |= 0; seed = seed + 0x6D2B79F5 | 0;
  let t = Math.imul(seed ^ seed >>> 15, 1 | seed);
  t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t;
  return ((t ^ t >>> 14) >>> 0) / 4294967296;
}
const uniform = (a, b) => a + (b - a) * rand();
const noise = s => s * Math.sqrt(-2 * Math.log(rand() || 1e-9)) * Math.cos(2 * Math.PI * rand());

// Giống computeLevel() trong water_level.cpp: hạ mức phải xuống dưới (ngưỡng - HYST_CM)
function computeLevel(h, prev) {
  if (h >= RED_CM) return 'red';
  if (prev === 'red' && h >= RED_CM - HYST_CM) return 'red';
  if (h >= YELLOW_CM) return 'yellow';
  if (prev === 'yellow' && h >= YELLOW_CM - HYST_CM) return 'yellow';
  return 'normal';
}

const steps = DAYS * 86400 / DT_S;
const t0 = Date.now() - DAYS * 86400 * 1000;
let soil = 30, flow = 0, h = 0.5, total = 0, level = 'normal';
let rainStart = uniform(0.5, 3) * 3600, rainDur = 0, rainPeak = 0;
const out = [];

for (let s = 0; s < steps; s++) {
  const t = s * DT_S;

  // Cơn mưa: hình chuông, dài 10-60 phút, cường độ đỉnh 5-60 mm/h, cách nhau 1-5 giờ
  if (t >= rainStart + rainDur) {
    rainStart = t + uniform(1, 5) * 3600;
    rainDur = uniform(10, 60) * 60;
    rainPeak = uniform(5, 60);
  }
  const phase = (t - rainStart) / rainDur;
  const rain = phase >= 0 && phase <= 1 ? rainPeak * Math.sin(Math.PI * phase) ** 2 : 0;

  // Đất thấm mưa thì ẩm lên, trời khô thì khô dần (khoảng 6 giờ)
  soil += DT_S * (rain * (100 - soil) / 100 * 0.004 - (soil - 20) / 21600);
  // Đất càng ướt thì nước chảy tràn càng nhiều; dòng chảy tới ống chậm khoảng 2 phút so với mưa
  const runoff = 0.25 * rain * (0.15 + 0.85 * (soil / 100) ** 2);
  flow += (runoff - flow) * DT_S / 120;
  // Bể: nước vào theo lưu lượng, thoát ra theo mực nước (hằng số thời gian 15 phút)
  h += DT_S * (0.0016 * flow - (h - 0.5) / 900);
  h = Math.min(Math.max(h, 0), H_TRAM_CM - 2);
  total += flow * DT_S / 60;

  // Số đo có nhiễu như cảm biến thật
  const hMeas = Math.max(0, h + noise(0.15));
  const rec = {
    received_at: new Date(t0 + t * 1000).toISOString(),
    event: 'report', level: computeLevel(hMeas, level),
    h_nuoc_cm: +hMeas.toFixed(1), d_loc_cm: +(H_TRAM_CM - hMeas).toFixed(1), h_tram_cm: H_TRAM_CM,
    flow_lpm: +Math.max(0, flow + noise(0.05)).toFixed(2), total_l: +total.toFixed(3),
    soil_pct: +Math.min(100, Math.max(0, soil + noise(0.5))).toFixed(1),
  };
  // Firmware gửi thêm bản tin alert ngay khi đổi mức
  if (rec.level !== level) out.push(JSON.stringify({ ...rec, event: 'alert' }));
  level = rec.level;
  out.push(JSON.stringify(rec));
}

const file = path.join(__dirname, 'synthetic.jsonl');
fs.writeFileSync(file, out.join('\n') + '\n');
const alerts = out.filter(l => l.includes('"alert"'));
console.log(`Da ghi ${out.length} ban tin (${DAYS} ngay) vao ${path.basename(file)}`);
console.log(`So lan doi muc: ${alerts.length}, len DO: ${alerts.filter(l => l.includes('"red"')).length}`);
