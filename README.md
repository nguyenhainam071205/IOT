# Trạm đo lũ ESP32

Firmware cho ESP32 đo ba đại lượng mỗi giây và gửi về web server qua WiFi:

| Đại lượng | Cảm biến | Kết quả |
|---|---|---|
| Mực nước | HC-SR04 (siêu âm) | `H_nuoc` (cm) và mức cảnh báo bình thường / vàng / đỏ |
| Lưu lượng | YF-S201 | `Q` (lít/phút) và tổng số lít |
| Độ ẩm đất | Module LM393 (ngõ ra analog) | `W` (%) |

Mục lục:

1. [Chạy trong Arduino IDE](#1-chạy-trong-arduino-ide)
2. [Code hoạt động thế nào](#2-code-hoạt-động-thế-nào)
3. [Các công thức](#3-các-công-thức)
4. [Dành cho người làm web server](#4-dành-cho-người-làm-web-server)

---

## 1. Chạy trong Arduino IDE

### 1.1. Cài đặt

1. Cài Arduino IDE (bản 2.x).
2. Cài board ESP32: **Tools → Board → Boards Manager**, tìm `esp32` (của Espressif Systems) và bấm Install.
   Nếu không thấy, thêm URL sau vào **File → Preferences → Additional boards manager URLs**:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Không cần cài thêm thư viện nào. `WiFi.h` và `HTTPClient.h` đã có sẵn trong gói board ESP32.

### 1.2. Mở project

Mở file `test.ino`. Arduino IDE tự mở tất cả file `.h` / `.cpp` cùng thư mục thành các tab, nạp từ tab `test` là nạp toàn bộ.

> Thư mục chứa code phải tên là `test` (trùng tên file `test.ino`), nếu không Arduino IDE sẽ đòi tạo thư mục mới.

### 1.3. Sửa cấu hình trong `config.h`

| Hằng số | Ý nghĩa |
|---|---|
| `WIFI_SSID`, `WIFI_PASS` | Tên và mật khẩu WiFi (ESP32 chỉ bắt được WiFi 2.4 GHz) |
| `SERVER_URL` | Địa chỉ web server nhận dữ liệu, ví dụ `http://192.168.1.100:3000/flood` |
| `H_TRAM_CM` | Chiều cao từ cảm biến HC-SR04 xuống đáy (mốc chuẩn), đo bằng thước |
| `TEMP_C` | Nhiệt độ môi trường, dùng để bù tốc độ âm thanh |
| `OFFSET_CM` | Độ lệch cố định của HC-SR04 sau khi so với thước |
| `ADC_KHO`, `ADC_UOT` | Hai mốc hiệu chuẩn độ ẩm đất (xem mục 3.5) |
| `YELLOW_CM`, `RED_CM`, `HYST_CM` | Ngưỡng cảnh báo và độ trễ |

### 1.4. Đấu nối

| Cảm biến | Chân cảm biến | Nối vào ESP32 |
|---|---|---|
| HC-SR04 | VCC | VIN (5V) |
| | GND | GND |
| | TRIG | GPIO 25 |
| | ECHO | qua chia áp 1k/2k → GPIO 26 |
| YF-S201 | Đỏ | VIN (5V) |
| | Đen | GND |
| | Vàng (xung) | qua chia áp 1k/2k → GPIO 27 |
| Độ ẩm đất | VCC | 3V3 |
| | GND | GND |
| | AO | GPIO 34 |

ECHO và dây xung xuất ra 5V, trong khi chân ESP32 chỉ chịu được 3.3V, nên phải qua mạch chia áp (điện trở 1k nối tiếp tín hiệu, 2k xuống GND):

```
V_out = 5V × 2k / (1k + 2k) ≈ 3.33V
```

### 1.5. Nạp và xem kết quả

1. **Tools → Board**: chọn `ESP32 Dev Module`. **Tools → Port**: chọn cổng COM của ESP32.
2. Bấm **Upload**. Nếu kẹt ở `Connecting...`, giữ nút BOOT trên board cho tới khi bắt đầu nạp.
3. Mở **Serial Monitor**, chọn **115200 baud**. Mỗi giây có một dòng:

```
D_raw: 24.7 | D_loc: 24.6 cm | H_nuoc: 5.4 cm | Muc: VANG | Luu luong: 4.00 L/phut (0.067 L) | Do am dat: 50.0 % (ADC 2450) | WiFi: OK
```

Các dòng khác có thể gặp:

| Dòng Serial | Ý nghĩa |
|---|---|
| `Muc: (dang khoi dong)` | Bộ lọc chưa đủ 5 mẫu (5 giây đầu), chưa cảnh báo và chưa gửi |
| `D_raw: --` | HC-SR04 mất echo trong chu kỳ này, `D_loc` giữ giá trị cũ |
| `Doi muc canh bao: VANG -> DO` | Mức cảnh báo vừa đổi |
| `POST alert -> 200` | Đã gửi lên server, số cuối là mã HTTP server trả về |
| `POST report -> -1` | Số âm: không kết nối được tới server (sai IP, server chưa chạy, bị tường lửa chặn) |
| `POST report bo qua: chua co WiFi` | Chưa có WiFi nên không gửi |

---

## 2. Code hoạt động thế nào

### 2.1. Các file

| File | Nhiệm vụ |
|---|---|
| `test.ino` | `setup()` và `loop()`: điều phối các bước đo và quyết định khi nào gửi |
| `config.h` | Chân GPIO, WiFi, URL server, mọi hằng số |
| `measurement.h` | `enum Level` và `struct Measurement` (số liệu của một chu kỳ đo) |
| `water_level.*` | HC-SR04: đo khoảng cách, lọc trung bình động, tính mực nước, mức cảnh báo |
| `flow.*` | YF-S201: đếm xung bằng ngắt, tính lưu lượng |
| `soil.*` | Đọc ADC và đổi ra % độ ẩm đất |
| `uplink.*` | Kết nối WiFi và gửi HTTP POST JSON |
| `display.*` | In một dòng số liệu ra Serial |

### 2.2. `setup()`

1. Mở Serial 115200 baud.
2. `waterLevelInit()`: cấu hình chân TRIG / ECHO.
3. `flowInit()`: gắn ngắt cạnh xuống trên GPIO 27 để đếm xung.
4. `uplinkInit()`: kết nối WiFi, chờ tối đa 15 giây. Nếu chưa được thì vẫn chạy tiếp và thử lại sau.

### 2.3. `loop()`

`uplinkMaintain()` chạy liên tục: nếu rớt WiFi thì thử kết nối lại mỗi 10 giây. Phần còn lại chạy mỗi 1 giây (`INTERVAL_MS`):

```
1) flowUpdate()        lấy số xung đếm được -> Q, tổng lít
2) waterLevelUpdate()  đo khoảng cách -> D_loc -> H_nuoc -> mức cảnh báo
3) soilUpdate()        đọc ADC -> W %
4) printStatus()       in một dòng ra Serial

   Nếu bộ lọc chưa đủ 5 mẫu: dừng ở đây.

5) Mức cảnh báo khác mức đang có hiệu lực  -> gửi POST "alert"
6) Chưa gửi lần nào, hoặc đã qua 5 phút    -> gửi POST "report"
```

---

## 3. Các công thức

### 3.1. Khoảng cách từ HC-SR04 (`water_level.cpp`)

Cảm biến phát xung siêu âm, code đo thời gian `t` (µs) từ lúc phát tới lúc nhận tiếng dội.

```
v = 331.3 + 0.606 × T          (m/s, T là nhiệt độ °C = TEMP_C)
v_cm = v / 10000               (đổi sang cm/µs)

D = t × v_cm / 2 + OFFSET_CM   (cm)
```

Chia 2 vì âm thanh đi tới mặt nước rồi quay về.

Ví dụ với `T = 30°C`: `v = 349.48 m/s = 0.034948 cm/µs`. Nếu `t = 1408 µs` thì `D = 1408 × 0.034948 / 2 ≈ 24.6 cm`.

Mỗi lần đo chờ echo tối đa 30 ms. Nếu mất echo, code thử lại tối đa 3 lần; vẫn không được thì chu kỳ đó không có mẫu mới.

### 3.2. Lọc trung bình động (`water_level.cpp`)

```
D_loc = (D[n] + D[n-1] + ... + D[n-N+1]) / N        với N = N_MA = 5
```

`D_loc` là trung bình của 5 mẫu gần nhất, giúp giảm nhiễu do sóng nước. Code giữ một tổng chạy: mỗi mẫu mới thì trừ mẫu cũ nhất, cộng mẫu mới, rồi chia cho số mẫu.

Trong 5 giây đầu chưa đủ mẫu, `D_loc` là trung bình của các mẫu đang có, và code chưa xét cảnh báo.

### 3.3. Mực nước và mức cảnh báo (`water_level.cpp`)

```
H_nuoc = H_tram − D_loc        (cm, H_tram = H_TRAM_CM = 30)
```

Cảm biến gắn ở độ cao `H_tram` so với đáy và nhìn xuống. Nước càng dâng thì `D_loc` càng nhỏ, `H_nuoc` càng lớn. Nếu kết quả âm thì lấy bằng 0.

Mức cảnh báo có độ trễ `HYST_CM = 0.5 cm` khi hạ mức, để không bị đổi qua lại liên tục khi nước dao động quanh ngưỡng:

| Mức | Lên mức khi | Hạ mức khi |
|---|---|---|
| ĐỎ (`red`) | `H_nuoc ≥ 8.0` | `H_nuoc < 7.5` |
| VÀNG (`yellow`) | `H_nuoc ≥ 4.0` | `H_nuoc < 3.5` |
| BÌNH THƯỜNG (`normal`) | còn lại | |

Ví dụ: `D_loc = 24.6 cm` thì `H_nuoc = 30 − 24.6 = 5.4 cm`, mức VÀNG.

### 3.4. Lưu lượng từ YF-S201 (`flow.cpp`)

Cảm biến phát xung khi nước chảy qua, mỗi xung làm ngắt tăng biến đếm lên 1. Mỗi chu kỳ, code lấy số xung rồi đặt lại về 0.

```
f = số_xung × 1000 / elapsed_ms     (Hz, số xung mỗi giây)

Q = f / 7.5                         (lít/phút, 7.5 = FLOW_K)

Tổng lít += (Q / 60) × (elapsed_ms / 1000)
```

- `f = 7.5 × Q` là đặc tính của YF-S201: 7.5 xung/giây ứng với 1 lít/phút.
- Code chia cho `elapsed_ms` thực tế chứ không giả định đúng 1000 ms, vì việc đo siêu âm và gửi HTTP có thể làm chu kỳ dài hơn 1 giây.
- `Q / 60` đổi lít/phút sang lít/giây, nhân với số giây của chu kỳ ra số lít chảy trong chu kỳ đó. Gộp lại tương đương 450 xung cho 1 lít.

Ví dụ: 30 xung trong 1000 ms thì `f = 30 Hz`, `Q = 30 / 7.5 = 4.00 lít/phút`, tổng cộng thêm `4 / 60 × 1 ≈ 0.067 lít`.

### 3.5. Độ ẩm đất (`soil.cpp`)

Code đọc ADC 10 lần (cách nhau 2 ms) rồi lấy trung bình để giảm nhiễu. ADC của ESP32 cho giá trị từ 0 tới 4095; với module này đất càng khô thì ADC càng cao.

```
W = (ADC_kho − ADC) / (ADC_kho − ADC_uot) × 100      (%)
```

Kết quả được giới hạn trong khoảng 0 tới 100.

Ví dụ với `ADC_kho = 3500`, `ADC_uot = 1400`, đọc được `ADC = 2450`: `W = (3500 − 2450) / (3500 − 1400) × 100 = 50%`.

**Hiệu chuẩn:** hai giá trị trong `config.h` chỉ là ví dụ. Để đầu dò ngoài không khí, đọc số `ADC` trên Serial và ghi vào `ADC_KHO`; nhúng đầu dò vào nước, đọc số `ADC` và ghi vào `ADC_UOT`.

---

## 4. Dành cho người làm web server

ESP32 là bên gửi (HTTP client). Web server chỉ cần mở một endpoint nhận `POST` với body JSON.

### 4.1. Request ESP32 gửi

```
POST http://<IP-server>:3000/flood
Content-Type: application/json
```

Đường dẫn và cổng lấy từ `SERVER_URL` trong `config.h`. Hai bên thống nhất rồi sửa hằng số này cho khớp.

Body:

```json
{
  "event": "alert",
  "level": "yellow",
  "h_nuoc_cm": 5.4,
  "d_loc_cm": 24.6,
  "h_tram_cm": 30.0,
  "flow_lpm": 4.00,
  "total_l": 0.067,
  "soil_pct": 50.0
}
```

| Trường | Kiểu | Ý nghĩa |
|---|---|---|
| `event` | chuỗi | `"alert"`: mức cảnh báo vừa đổi. `"report"`: báo cáo định kỳ |
| `level` | chuỗi | `"normal"`, `"yellow"` hoặc `"red"` |
| `h_nuoc_cm` | số | Mực nước (cm), luôn ≥ 0 |
| `d_loc_cm` | số | Khoảng cách đã lọc từ cảm biến tới mặt nước (cm) |
| `h_tram_cm` | số | Chiều cao lắp cảm biến (cm), hằng số cấu hình |
| `flow_lpm` | số | Lưu lượng (lít/phút) của 1 giây gần nhất |
| `total_l` | số | Tổng số lít từ lúc ESP32 bật nguồn |
| `soil_pct` | số | Độ ẩm đất (%), từ 0 tới 100 |

### 4.2. Khi nào ESP32 gửi

| `event` | Thời điểm |
|---|---|
| `report` | Lần đầu khoảng 5 giây sau khi khởi động (lúc bộ lọc đủ mẫu), sau đó mỗi 5 phút (`REPORT_INTERVAL_MS`) |
| `alert` | Ngay khi mức cảnh báo đổi, cả lúc tăng lẫn lúc hạ (ví dụ `yellow` về `normal`) |

Có thể nhận `alert` và `report` liền nhau trong cùng một giây.

### 4.3. Server cần trả về gì

- Trả mã **2xx** (ví dụ `200`) là ESP32 coi như gửi thành công. ESP32 không đọc nội dung body trả về.
- Trả lời trong vòng **5 giây**. Quá thời gian này ESP32 coi là lỗi, và trong lúc chờ thì ESP32 tạm ngừng đo.

### 4.4. Những điều cần biết khi thiết kế server

- **Không có thời gian trong JSON.** ESP32 không có đồng hồ thực, server tự gắn thời gian lúc nhận.
- **Không có mã thiết bị.** Nếu sau này có nhiều trạm, cần thêm một trường `device_id` vào JSON trong `uplink.cpp`.
- **Không gửi lại khi lỗi.** Nếu mất WiFi hoặc server không trả lời, bản tin đó bị bỏ. Một `alert` bị mất sẽ không được gửi lại; server chỉ biết mức mới qua `report` kế tiếp (tối đa 5 phút sau). Vì vậy nên lấy `level` từ mọi bản tin, không chỉ từ `alert`.
- **`total_l` về 0 khi ESP32 khởi động lại.** Nếu thấy giá trị nhỏ hơn lần trước thì trạm vừa reset.
- **`flow_lpm` là giá trị tức thời** của 1 giây ngay trước khi gửi, không phải trung bình 5 phút. Muốn biết lượng nước giữa hai bản tin thì lấy hiệu `total_l`.
- **Chỉ có HTTP, không có HTTPS**, và không có xác thực.
- **Phát hiện trạm mất kết nối:** nếu quá khoảng 6 phút không nhận được bản tin nào thì coi như trạm offline.

### 4.5. Điều kiện mạng

- Máy chạy server và ESP32 phải cùng mạng WiFi/LAN (hoặc server có IP công khai).
- Server phải lắng nghe trên `0.0.0.0`, không phải chỉ `localhost` / `127.0.0.1`.
- Mở cổng (ví dụ 3000) trên tường lửa của máy chạy server.
- Lấy IP của máy chạy server bằng `ipconfig` (Windows) rồi ghi vào `SERVER_URL`. Nên đặt IP tĩnh cho máy này để IP không đổi.

### 4.6. Ví dụ server tối thiểu (Node.js + Express)

```js
const express = require('express');
const app = express();
app.use(express.json());

app.post('/flood', (req, res) => {
  const data = { ...req.body, received_at: new Date().toISOString() };
  console.log(data);          // thay bằng lưu database / đẩy lên giao diện
  res.sendStatus(200);
});

app.listen(3000, '0.0.0.0', () => console.log('Dang nghe cong 3000'));
```

### 4.7. Thử server khi chưa có ESP32

Gửi một bản tin giả bằng PowerShell:

```powershell
$body = '{"event":"alert","level":"yellow","h_nuoc_cm":5.4,"d_loc_cm":24.6,"h_tram_cm":30.0,"flow_lpm":4.00,"total_l":0.067,"soil_pct":50.0}'
Invoke-RestMethod -Uri http://localhost:3000/flood -Method Post -ContentType 'application/json' -Body $body
```

Khi chạy với ESP32 thật, dòng `POST report -> 200` trên Serial Monitor xác nhận server đã nhận được.
