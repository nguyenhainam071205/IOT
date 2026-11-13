// Huấn luyện mô hình dự báo mực nước, lưu lượng, độ ẩm đất; ghi ra model.json cho server.js dùng.
//   node train.js                        pre-train từ đầu trên synthetic.jsonl
//   node train.js --finetune data.jsonl  fine-tune model.json hiện có trên dữ liệu trạm thật
// Chia dữ liệu theo thời gian: 80% đầu để train, 20% cuối để kiểm tra.
const fs = require('fs');
const path = require('path');
const fc = require('./forecast');

const MODEL_FILE = path.join(__dirname, 'model.json');
const finetune = process.argv[2] === '--finetune';
const dataFile = path.resolve(__dirname, finetune ? (process.argv[3] || 'data.jsonl') : 'synthetic.jsonl');

if (!fs.existsSync(dataFile)) {
  console.error(`Khong tim thay ${dataFile}` + (finetune ? '' : '. Chay "node generate-data.js" truoc.'));
  process.exit(1);
}
const recs = fs.readFileSync(dataFile, 'utf8').split('\n').filter(Boolean).map(l => JSON.parse(l))
  .sort((a, b) => Date.parse(a.received_at) - Date.parse(b.received_at));
const { X, Y, H } = fc.buildDataset(recs);
if (X.length < 100) {
  console.error(`Chi co ${X.length} mau trong ${path.basename(dataFile)}, can it nhat 100 (khoang 25 phut du lieu lien tuc).`);
  process.exit(1);
}
const cut = Math.floor(X.length * 0.8);
const train = [X.slice(0, cut), Y.slice(0, cut)];
const test = [X.slice(cut), Y.slice(cut), H.slice(cut)];

const UNIT = { h_nuoc_cm: 'cm', flow_lpm: 'L/phut', soil_pct: '%' };
const NAME = { h_nuoc_cm: 'muc nuoc', flow_lpm: 'luu luong', soil_pct: 'do am dat' };

// In sai số từng đại lượng × từng mốc so với cách "giữ nguyên giá trị hiện tại"
const report = (title, model) => {
  const { mae, base } = fc.evaluate(model, ...test);
  console.log(title);
  fc.TARGETS.forEach((t, ti) => fc.HORIZONS_S.forEach((s, h) => {
    const k = ti * fc.HORIZONS_S.length + h;
    console.log(`  ${NAME[t]} +${s / 60} phut: sai so ${mae[k].toFixed(3)} ${UNIT[t]} (giu nguyen: ${base[k].toFixed(3)} ${UNIT[t]})`);
  }));
  return { mae, base };
};

let model;
if (finetune) {
  model = JSON.parse(fs.readFileSync(MODEL_FILE, 'utf8'));
  if (!fc.isCompatible(model)) {
    console.error('model.json la dinh dang cu (chi du bao muc nuoc). Chay "node train.js" de pre-train lai truoc.');
    process.exit(1);
  }
  report('Truoc fine-tune:', model);
  fc.train(model, ...train, 100, 0.02);   // ít vòng, bước nhỏ: chỉnh nhẹ, không quên những gì học từ dữ liệu giả lập
} else {
  model = fc.newModel(train[0]);
  fc.train(model, ...train, 1500, 0.1);   // sai số kiểm tra hết giảm sau khoảng 1500 vòng
}
const { mae, base } = report(finetune ? 'Sau fine-tune:' : 'Sau pre-train:', model);
console.log(`${X.length} mau tu ${path.basename(dataFile)}`);

// Tự kiểm tra: mọi đầu ra phải đoán tốt hơn việc giả định giá trị đứng yên, nếu không thì không ghi đè model.json
const bad = [];
mae.forEach((m, k) => {
  if (m >= base[k]) bad.push(`${NAME[fc.TARGETS[Math.floor(k / fc.HORIZONS_S.length)]]} +${fc.HORIZONS_S[k % fc.HORIZONS_S.length] / 60} phut`);
});
if (bad.length) {
  console.error(`Mo hinh khong tot hon cach giu nguyen o: ${bad.join(', ')}. KHONG ghi model.json`);
  process.exit(1);
}
fs.writeFileSync(MODEL_FILE, JSON.stringify(model, null, 2));
console.log(`Da ghi ${path.basename(MODEL_FILE)}`);
