// Sinh dữ liệu giả lập của trạm đo lũ, cùng định dạng với data.jsonl mà server ghi.
// Mô phỏng: mưa -> đất ẩm dần -> nước chảy tràn qua ống (YF-S201) -> mực nước trong bể dâng.
// Chạy: node generate-data.js [số ngày=30] [seed=1]   -> ghi ra synthetic.jsonl
const fs = require('fs');
const path = require('path');

const DAYS = Number(process.argv[2]) || 30;
let seed = Number(process.argv[3]) || 1;

// Cùng hằng số với firmware/config.h
const H_TRAM_CM = 30, YELLOW_CM = 4, RED_CM = 8, HYST_CM = 0.5;
const FLOW_YELLOW_LPM = 10, FLOW_RED_LPM = 20, FLOW_HYST_LPM = 1;
const SOIL_YELLOW_PCT = 70, SOIL_RED_PCT = 90, SOIL_HYST_PCT = 3;
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

// Giống levelFromValue() trong firmware/alert.cpp: hạ mức phải xuống dưới (ngưỡng - độ trễ)
function levelFromValue(v, yellow, red, hyst, prev) {
  if (v >= red) return 'red';
  if (prev === 'red' && v >= red - hyst) return 'red';
  if (v >= yellow) return 'yellow';
  if (prev === 'yellow' && v >= yellow - hyst) return 'yellow';
  return 'normal';
}
// Giống flashFloodLevel() trong firmware/alert.cpp
const RANK = { normal: 0, yellow: 1, red: 2 };
function flashFloodLevel(flow, soil) {
  if (RANK[flow] >= 1 && RANK[soil] >= 1) return 'red';
  if (flow === 'red' || soil === 'red') return 'yellow';
  return 'normal';
}

const steps = DAYS * 86400 / DT_S;
const t0 = Date.now() - DAYS * 86400 * 1000;
let soil = 30, flow = 0, h = 0.5, total = 0;
let lvWater = 'normal', lvFlow = 'normal', lvSoil = 'normal', lvFlash = 'normal';   // mức đang có hiệu lực
let rainStart = uniform(0.5, 3) * 3600, rainDur = 0, rainPeak = 0;
const out = [];

for (let s = 0; s < steps; s++) {
  const t = s * DT_S;

  // Cơn mưa: hình chuông, dài 10-60 phút, cường độ đỉnh 5-60 mm/h (hiếm hơn: 80-150), cách nhau 1-5 giờ
  if (t >= rainStart + rainDur) {
    rainStart = t + uniform(1, 5) * 3600;
    rainDur = uniform(10, 60) * 60;
    // Khoảng 1 cơn trong 10 là bão lớn (80-150 mm/h): chỉ cơn như vậy mới đưa lưu lượng qua mốc đỏ
    rainPeak = rand() < 0.1 ? uniform(80, 150) : uniform(5, 60);
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
  const flowMeas = +Math.max(0, flow + noise(0.05)).toFixed(2);
  const soilMeas = +Math.min(100, Math.max(0, soil + noise(0.5))).toFixed(1);

  // Mức cảnh báo như firmware (alert.cpp): Q và W so với mốc, lũ quét ghép từ hai mức đó, mức tổng hợp = cao nhất
  const waterLevel = levelFromValue(hMeas, YELLOW_CM, RED_CM, HYST_CM, lvWater);
  const flowLevel = levelFromValue(flowMeas, FLOW_YELLOW_LPM, FLOW_RED_LPM, FLOW_HYST_LPM, lvFlow);
  const soilLevel = levelFromValue(soilMeas, SOIL_YELLOW_PCT, SOIL_RED_PCT, SOIL_HYST_PCT, lvSoil);
  const flashLevel = flashFloodLevel(flowLevel, soilLevel);
  const level = RANK[waterLevel] >= RANK[flashLevel] ? waterLevel : flashLevel;

  const rec = {
    received_at: new Date(t0 + t * 1000).toISOString(),
    event: 'report', level,
    water_level: waterLevel, flow_level: flowLevel, soil_level: soilLevel, flash_level: flashLevel,
    h_nuoc_cm: +hMeas.toFixed(1), d_loc_cm: +(H_TRAM_CM - hMeas).toFixed(1), h_tram_cm: H_TRAM_CM,
    flow_lpm: flowMeas, flow_avg_lpm: flowMeas, total_l: +total.toFixed(3),
    soil_pct: soilMeas,
  };
  // Firmware gửi thêm bản tin alert ngay khi bất kỳ mức nào đổi
  if (waterLevel !== lvWater || flowLevel !== lvFlow || soilLevel !== lvSoil || flashLevel !== lvFlash) {
    out.push(JSON.stringify({ ...rec, event: 'alert' }));
  }
  lvWater = waterLevel; lvFlow = flowLevel; lvSoil = soilLevel; lvFlash = flashLevel;
  out.push(JSON.stringify(rec));
}

const file = path.join(__dirname, 'synthetic.jsonl');
fs.writeFileSync(file, out.join('\n') + '\n');
const alerts = out.filter(l => l.includes('"alert"'));
console.log(`Da ghi ${out.length} ban tin (${DAYS} ngay) vao ${path.basename(file)}`);
console.log(`So lan doi muc: ${alerts.length}, len DO: ${alerts.filter(l => l.includes('"red"')).length}`);
// Thống kê để chắc chắn dữ liệu huấn luyện có sự kiện ở từng mức (số bản tin định kỳ, mỗi bản tin = 5 giây)
const reports = out.map(l => JSON.parse(l)).filter(r => r.event === 'report');
const count = f => reports.filter(f).length;
console.log(`Ban tin Q >= vang: ${count(r => r.flow_lpm >= FLOW_YELLOW_LPM)}, Q >= do: ${count(r => r.flow_lpm >= FLOW_RED_LPM)}`);
console.log(`Ban tin W >= vang: ${count(r => r.soil_pct >= SOIL_YELLOW_PCT)}, W >= do: ${count(r => r.soil_pct >= SOIL_RED_PCT)}`);
console.log(`Ban tin lu quet DO: ${count(r => r.flash_level === 'red')}, VANG: ${count(r => r.flash_level === 'yellow')}`);
