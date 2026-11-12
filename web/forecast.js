// Dự báo mực nước sau 5 và 10 phút. Dùng chung cho train.js (huấn luyện) và server.js (dự báo).
// Mô hình: hồi quy tuyến tính dự báo độ thay đổi mực nước, huấn luyện bằng gradient descent
// để fine-tune tiếp được trên dữ liệu thật mà không phải train lại từ đầu.
// shortcut: mô hình tuyến tính, đủ cho bể nước đơn giản; nếu sai số trên dữ liệu thật vẫn lớn thì đổi sang MLP/LightGBM.

const LAGS_S = [30, 60, 120, 300];   // nhìn lại 5 phút gần nhất
const HORIZONS_S = [300, 600];       // dự báo +5 phút và +10 phút
const TOL_MS = 15000;                // bản tin cách mốc cần tìm quá 15 s coi như thiếu dữ liệu

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

// Đặc trưng tại bản tin i: mực nước, lưu lượng, độ ẩm, tốc độ dâng trong các khoảng LAGS_S, thay đổi lưu lượng 1 phút
function features(recs, times, i) {
  const r = recs[i];
  const f = [r.h_nuoc_cm, r.flow_lpm, r.soil_pct];
  let j60 = -1;
  for (const lag of LAGS_S) {
    const j = nearest(times, times[i] - lag * 1000);
    if (j < 0) return null;
    if (lag === 60) j60 = j;
    f.push(r.h_nuoc_cm - recs[j].h_nuoc_cm);
  }
  f.push(r.flow_lpm - recs[j60].flow_lpm);
  return f;
}

// Trả về mảng mực nước dự báo (cm) cho từng HORIZONS_S
function predict(model, f, hTram) {
  return model.w.map((w, k) => {
    let delta = model.b[k];
    for (let d = 0; d < f.length; d++) delta += w[d] * (f[d] - model.mean[d]) / model.std[d];
    return Math.min(Math.max(f[0] + delta, 0), hTram);
  });
}

// Dự báo từ lịch sử bản tin (đã sắp theo thời gian); null nếu thiếu dữ liệu 5 phút gần nhất
function forecastLatest(model, recs) {
  if (!model || recs.length === 0) return null;
  const times = recs.map(r => Date.parse(r.received_at));
  const f = features(recs, times, recs.length - 1);
  return f && predict(model, f, recs[recs.length - 1].h_tram_cm);
}

// Bộ dữ liệu (X, Y) từ chuỗi bản tin. Y[k] = mực nước sau HORIZONS_S[k] trừ mực nước hiện tại.
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
    Y.push(js.map(j => recs[j].h_nuoc_cm - recs[i].h_nuoc_cm));
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
  return {
    horizons_s: HORIZONS_S, mean, std: std.map(s => Math.sqrt(s) || 1),
    w: HORIZONS_S.map(() => Array(d).fill(0)), b: HORIZONS_S.map(() => 0),
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

// Sai số tuyệt đối trung bình (cm) của mô hình và của cách "giữ nguyên mực nước hiện tại" (mốc so sánh)
function evaluate(model, X, Y, H) {
  const mae = HORIZONS_S.map(() => 0), base = HORIZONS_S.map(() => 0);
  X.forEach((x, i) => {
    predict(model, x, H[i]).forEach((p, k) => {
      mae[k] += Math.abs(p - (x[0] + Y[i][k])) / X.length;
      base[k] += Math.abs(Y[i][k]) / X.length;
    });
  });
  return { mae, base };
}

module.exports = { HORIZONS_S, forecastLatest, buildDataset, newModel, train, evaluate };
