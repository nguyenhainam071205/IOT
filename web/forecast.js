// Dự báo mực nước, lưu lượng và độ ẩm đất sau 5 và 10 phút. Dùng chung cho train.js (huấn luyện) và server.js (dự báo).
// Mô hình: hồi quy tuyến tính dự báo độ thay đổi của từng đại lượng, huấn luyện bằng gradient descent
// để fine-tune tiếp được trên dữ liệu thật mà không phải train lại từ đầu.
// shortcut: mô hình tuyến tính, đủ cho bể nước đơn giản; nếu sai số trên dữ liệu thật vẫn lớn thì đổi sang MLP/LightGBM.

const TARGETS = ['h_nuoc_cm', 'flow_lpm', 'soil_pct'];   // đại lượng được dự báo; 3 đặc trưng đầu của features() cũng theo đúng thứ tự này
const LAGS_S = [30, 60, 120, 300];   // nhìn lại 5 phút gần nhất
const HORIZONS_S = [300, 600];       // dự báo +5 phút và +10 phút
const TOL_MS = 15000;                // bản tin cách mốc cần tìm quá 15 s coi như thiếu dữ liệu
const NH = HORIZONS_S.length;        // đầu ra thứ k = đại lượng floor(k / NH), mốc thời gian k % NH

// Chỉ số bản tin gần thời điểm t nhất (times tăng dần), -1 nếu không có bản tin nào đủ gần
function nearest(times, t) {
  let lo = 0, hi = times.length - 1;
  while (lo < hi) {
    const mid = (lo + hi) >> 1;
    if (times[mid] < t) lo = mid + 1; else hi = mid;
  }
  if (lo > 0 && Math.abs(times[lo - 1] - t) < Math.abs(times[lo] - t)) lo--;
  return Math.abs(times[lo] - t) <= TOL_MS ? lo : -1;
}

// Đặc trưng tại bản tin i: mực nước, lưu lượng, độ ẩm; tốc độ dâng trong các khoảng LAGS_S;
// thay đổi lưu lượng trong 1 và 5 phút; thay đổi độ ẩm trong 5 phút
function features(recs, times, i) {
  const r = recs[i];
  const f = [r.h_nuoc_cm, r.flow_lpm, r.soil_pct];
  let j60 = -1, j300 = -1;
  for (const lag of LAGS_S) {
    const j = nearest(times, times[i] - lag * 1000);
    if (j < 0) return null;
    if (lag === 60) j60 = j;
    if (lag === 300) j300 = j;
    f.push(r.h_nuoc_cm - recs[j].h_nuoc_cm);
  }
  f.push(r.flow_lpm - recs[j60].flow_lpm);
  f.push(r.flow_lpm - recs[j300].flow_lpm);
  f.push(r.soil_pct - recs[j300].soil_pct);
  return f;
}

// Trả về { h_nuoc_cm: [...], flow_lpm: [...], soil_pct: [...] }, mỗi mảng theo từng HORIZONS_S
function predict(model, f, hTram) {
  const hi = [hTram, Infinity, 100];   // cận trên vật lý của từng đại lượng (cận dưới đều là 0)
  const out = {};
  TARGETS.forEach(t => { out[t] = []; });
  model.w.forEach((w, k) => {
    const t = Math.floor(k / NH);
    let delta = model.b[k];
    for (let d = 0; d < f.length; d++) delta += w[d] * (f[d] - model.mean[d]) / model.std[d];
    out[TARGETS[t]].push(Math.min(Math.max(f[t] + delta, 0), hi[t]));
  });
  return out;
}

// Dự báo từ lịch sử bản tin (đã sắp theo thời gian); null nếu thiếu dữ liệu 5 phút gần nhất
function forecastLatest(model, recs) {
  if (!model || recs.length === 0) return null;
  const times = recs.map(r => Date.parse(r.received_at));
  const f = features(recs, times, recs.length - 1);
  return f && predict(model, f, recs[recs.length - 1].h_tram_cm);
}

// Mô hình lưu trong model.json có đúng định dạng hiện tại (3 đại lượng × 2 mốc) không
function isCompatible(model) {
  return !!model && Array.isArray(model.targets) && model.targets.join() === TARGETS.join()
    && Array.isArray(model.w) && model.w.length === TARGETS.length * NH;
}

// Bộ dữ liệu (X, Y) từ chuỗi bản tin. Y[k] = giá trị đại lượng sau mốc thời gian trừ giá trị hiện tại.
function buildDataset(recs) {
  recs = recs.filter(r => r.event === 'report');   // bản tin alert trùng thời điểm với report, bỏ cho khỏi lặp
  const times = recs.map(r => Date.parse(r.received_at));
  const X = [], Y = [], H = [];
  for (let i = 0; i < recs.length; i++) {
    const f = features(recs, times, i);
    if (!f) continue;
    const js = HORIZONS_S.map(s => nearest(times, times[i] + s * 1000));
    if (js.includes(-1)) continue;
    X.push(f);
    Y.push(TARGETS.flatMap(t => js.map(j => recs[j][t] - recs[i][t])));
    H.push(recs[i].h_tram_cm);
  }
  return { X, Y, H };
}

// Mô hình mới: chuẩn hoá đặc trưng theo dữ liệu pre-train, giữ nguyên khi fine-tune
function newModel(X) {
  const d = X[0].length;
  const mean = Array(d).fill(0), std = Array(d).fill(0);
  for (const x of X) x.forEach((v, j) => mean[j] += v / X.length);
  for (const x of X) x.forEach((v, j) => std[j] += (v - mean[j]) ** 2 / X.length);
  const outputs = TARGETS.length * NH;
  return {
    targets: TARGETS, horizons_s: HORIZONS_S, mean, std: std.map(s => Math.sqrt(s) || 1),
    w: Array.from({ length: outputs }, () => Array(d).fill(0)), b: Array(outputs).fill(0),
  };
}

// Gradient descent toàn bộ dữ liệu, tối thiểu sai số bình phương. Sửa model tại chỗ.
function train(model, X, Y, epochs, lr) {
  const n = X.length, d = X[0].length;
  const Z = X.map(x => x.map((v, j) => (v - model.mean[j]) / model.std[j]));
  for (let e = 0; e < epochs; e++) {
    model.w.forEach((w, k) => {
      const gw = Array(d).fill(0);
      let gb = 0;
      for (let i = 0; i < n; i++) {
        let err = model.b[k] - Y[i][k];
        for (let j = 0; j < d; j++) err += w[j] * Z[i][j];
        for (let j = 0; j < d; j++) gw[j] += err * Z[i][j] / n;
        gb += err / n;
      }
      for (let j = 0; j < d; j++) w[j] -= lr * gw[j];
      model.b[k] -= lr * gb;
    });
  }
}

// Sai số tuyệt đối trung bình của mô hình (mae) và của cách "giữ nguyên giá trị hiện tại" (base, mốc so sánh).
// Cả hai là mảng độ dài TARGETS.length × NH, đơn vị theo từng đại lượng (cm, L/phút, %).
function evaluate(model, X, Y, H) {
  const mae = Y[0].map(() => 0), base = Y[0].map(() => 0);
  X.forEach((x, i) => {
    const p = predict(model, x, H[i]);
    TARGETS.forEach((t, ti) => p[t].forEach((v, h) => {
      const k = ti * NH + h;
      mae[k] += Math.abs(v - (x[ti] + Y[i][k])) / X.length;
      base[k] += Math.abs(Y[i][k]) / X.length;
    }));
  });
  return { mae, base };
}

module.exports = { TARGETS, HORIZONS_S, forecastLatest, isCompatible, buildDataset, newModel, train, evaluate };
