// Huấn luyện mô hình dự báo mực nước, ghi ra model.json cho server.js dùng.
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

const report = (title, model) => {
  const { mae, base } = fc.evaluate(model, ...test);
  console.log(title);
  fc.HORIZONS_S.forEach((s, k) =>
    console.log(`  +${s / 60} phut: sai so ${mae[k].toFixed(2)} cm (giu nguyen muc nuoc: ${base[k].toFixed(2)} cm)`));
  return { mae, base };
};

let model;
if (finetune) {
  model = JSON.parse(fs.readFileSync(MODEL_FILE, 'utf8'));
  report('Truoc fine-tune:', model);
  fc.train(model, ...train, 100, 0.02);   // ít vòng, bước nhỏ: chỉnh nhẹ, không quên những gì học từ dữ liệu giả lập
} else {
  model = fc.newModel(train[0]);
  fc.train(model, ...train, 1500, 0.1);   // sai số kiểm tra hết giảm sau khoảng 1500 vòng
}
const { mae, base } = report(finetune ? 'Sau fine-tune:' : 'Sau pre-train:', model);
console.log(`${X.length} mau tu ${path.basename(dataFile)}`);

// Tự kiểm tra: mô hình phải đoán tốt hơn việc giả định mực nước đứng yên, nếu không thì không ghi đè model.json
if (mae.some((m, k) => m >= base[k])) {
  console.error('Mo hinh khong tot hon cach giu nguyen muc nuoc, KHONG ghi model.json');
  process.exit(1);
}
fs.writeFileSync(MODEL_FILE, JSON.stringify(model, null, 2));
console.log(`Da ghi ${path.basename(MODEL_FILE)}`);
