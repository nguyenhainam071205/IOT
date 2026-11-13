# Trạm đo lũ ESP32

ESP32 đo ba đại lượng mỗi giây rồi gửi về web server qua WiFi. Web server lưu dữ liệu, vẽ dashboard, cảnh báo lũ quét (lưu lượng + độ ẩm đất) và dự báo mực nước, lưu lượng, độ ẩm đất 5 và 10 phút tới.

| Đại lượng | Cảm biến | Kết quả |
|---|---|---|
| Mực nước | HC-SR04 (siêu âm) | `H_nuoc` (cm) và mức cảnh báo bình thường / vàng / đỏ |
| Lưu lượng | YF-S201 | `Q` (lít/phút) và tổng số lít |
| Độ ẩm đất | Module LM393 (ngõ ra analog) | `W` (%) |

| Thư mục | Nội dung |
|---|---|
| `firmware/` | Code Arduino nạp vào ESP32 |
| `web/` | Web server Node.js: nhận dữ liệu, dashboard, mô hình dự báo |

Mục lục:

1. [Chạy trong Arduino IDE](#1-chạy-trong-arduino-ide)
2. [Code hoạt động thế nào](#2-code-hoạt-động-thế-nào)
3. [Các công thức](#3-các-công-thức)
4. [Web server](#4-web-server)
5. [Dự báo mực nước, lưu lượng và độ ẩm đất](#5-dự-báo-mực-nước-lưu-lượng-và-độ-ẩm-đất)

## 1. Chạy trong Arduino IDE

### 1.1. Cài đặt

1. Cài Arduino IDE (bản 2.x).
2. Cài board ESP32: vào **Tools → Board → Boards Manager**, tìm `esp32` (của Espressif Systems) rồi bấm Install.
   Nếu không thấy, thêm URL sau vào **File → Preferences → Additional boards manager URLs**:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Không cần cài thư viện nào khác, vì `WiFi.h` và `HTTPClient.h` có sẵn trong gói board ESP32.

### 1.2. Mở project

Mở file `firmware/firmware.ino`. Arduino IDE sẽ mở mọi file `.h` / `.cpp` trong cùng thư mục thành các tab, và nạp từ tab `firmware` là nạp cả project.

Thư mục chứa code phải tên là `firmware`, trùng với tên file `firmware.ino`. Nếu khác tên, Arduino IDE sẽ đòi tạo thư mục mới.

### 1.3. Sửa cấu hình trong `config.h`

| Hằng số | Ý nghĩa |
|---|---|
| `WIFI_SSID`, `WIFI_PASS` | Tên và mật khẩu WiFi (ESP32 chỉ bắt được WiFi 2.4 GHz) |
| `SERVER_URL` | Địa chỉ web server nhận dữ liệu, ví dụ `http://192.168.1.100:3000/flood` |
| `H_TRAM_CM` | Chiều cao từ cảm biến HC-SR04 xuống đáy (mốc chuẩn), đo bằng thước |
| `TEMP_C` | Nhiệt độ môi trường, dùng để bù tốc độ âm thanh |
| `OFFSET_CM` | Độ lệch cố định của HC-SR04 sau khi so với thước |
| `ADC_KHO`, `ADC_UOT` | Hai mốc hiệu chuẩn độ ẩm đất (xem mục 3.5) |
| `YELLOW_CM`, `RED_CM`, `HYST_CM` | Ngưỡng cảnh báo mực nước và độ trễ |
| `FLOW_YELLOW_LPM`, `FLOW_RED_LPM`, `FLOW_HYST_LPM`, `FLOW_MA_N` | Ngưỡng cảnh báo lưu lượng, độ trễ và số mẫu trung bình (mục 3.6) |
| `SOIL_YELLOW_PCT`, `SOIL_RED_PCT`, `SOIL_HYST_PCT` | Ngưỡng cảnh báo độ ẩm đất và độ trễ (mục 3.6) |

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

Chân ECHO và dây xung xuất ra 5V, còn chân ESP32 chỉ chịu được 3.3V. Vì vậy hai tín hiệu này phải đi qua mạch chia áp, gồm điện trở 1k nối tiếp tín hiệu và 2k xuống GND:

```
V_out = 5V × 2k / (1k + 2k) ≈ 3.33V
```

### 1.5. Nạp và xem kết quả

1. Ở **Tools → Board** chọn `ESP32 Dev Module`, ở **Tools → Port** chọn cổng COM của ESP32.
2. Bấm **Upload**. Nếu kẹt ở `Connecting...` thì giữ nút BOOT trên board cho tới khi bắt đầu nạp.
3. Mở **Serial Monitor** ở 115200 baud. Mỗi giây sẽ có một dòng:

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

## 2. Code hoạt động thế nào

### 2.1. Các file trong `firmware/`

| File | Nhiệm vụ |
|---|---|
| `firmware.ino` | `setup()` và `loop()`: điều phối các bước đo và quyết định khi nào gửi |
| `config.h` | Chân GPIO, WiFi, URL server, mọi hằng số |
| `measurement.h` | `enum Level` và `struct Measurement` (số liệu của một chu kỳ đo) |
| `water_level.*` | HC-SR04: đo khoảng cách, lọc trung bình động, tính mực nước, mức cảnh báo |
| `flow.*` | YF-S201: đếm xung bằng ngắt, tính lưu lượng |
| `soil.*` | Đọc ADC và đổi ra % độ ẩm đất |
| `alert.*` | Mức cảnh báo lưu lượng, độ ẩm đất, lũ quét và mức tổng hợp |
| `uplink.*` | Kết nối WiFi và gửi HTTP POST JSON |
| `display.*` | In một dòng số liệu ra Serial |

### 2.2. `setup()`

1. Mở Serial 115200 baud.
2. `waterLevelInit()` cấu hình chân TRIG / ECHO.
3. `flowInit()` gắn ngắt cạnh xuống trên GPIO 27 để đếm xung.
4. `uplinkInit()` kết nối WiFi và chờ tối đa 15 giây. Nếu chưa kết nối được thì vẫn chạy tiếp và thử lại sau.

### 2.3. `loop()`

`uplinkMaintain()` chạy liên tục và thử kết nối lại WiFi mỗi 10 giây nếu bị rớt. Phần còn lại chạy mỗi 1 giây (`INTERVAL_MS`):

```
1) flowUpdate()        lấy số xung đếm được -> Q, tổng lít
2) waterLevelUpdate()  đo khoảng cách -> D_loc -> H_nuoc -> mức cảnh báo
3) soilUpdate()        đọc ADC -> W %
4) alertUpdate()       Q trung bình, mức Q / W / lũ quét, mức tổng hợp
5) printStatus()       in một dòng ra Serial

   Nếu bộ lọc chưa đủ 5 mẫu: dừng ở đây.

6) Có mức nào (nước, Q, W, lũ quét) khác mức đang có hiệu lực -> gửi POST "alert"
7) Chưa gửi lần nào, hoặc đã qua 5 giây                       -> gửi POST "report"
```

## 3. Các công thức

### 3.1. Khoảng cách từ HC-SR04 (`water_level.cpp`)

Cảm biến phát xung siêu âm, và code đo thời gian `t` (µs) từ lúc phát tới lúc nhận tiếng dội.

```
v = 331.3 + 0.606 × T          (m/s, T là nhiệt độ °C = TEMP_C)
v_cm = v / 10000               (đổi sang cm/µs)

D = t × v_cm / 2 + OFFSET_CM   (cm)
```

Phải chia 2 vì âm thanh đi tới mặt nước rồi mới quay về.

Ví dụ với `T = 30°C`: `v = 349.48 m/s = 0.034948 cm/µs`. Nếu `t = 1408 µs` thì `D = 1408 × 0.034948 / 2 ≈ 24.6 cm`.

Mỗi lần đo chờ echo tối đa 30 ms. Mất echo thì code thử lại, tối đa 3 lần; nếu vẫn không được thì chu kỳ đó không có mẫu mới.

### 3.2. Lọc trung bình động (`water_level.cpp`)

```
D_loc = (D[n] + D[n-1] + ... + D[n-N+1]) / N        với N = N_MA = 5
```

`D_loc` là trung bình của 5 mẫu gần nhất, giúp giảm nhiễu do sóng nước. Code giữ một tổng chạy: mỗi khi có mẫu mới thì trừ mẫu cũ nhất, cộng mẫu mới, rồi chia cho số mẫu.

Trong 5 giây đầu chưa đủ mẫu, `D_loc` là trung bình của các mẫu đang có và code chưa xét cảnh báo.

### 3.3. Mực nước và mức cảnh báo (`water_level.cpp`)

```
H_nuoc = H_tram − D_loc        (cm, H_tram = H_TRAM_CM = 30)
```

Cảm biến gắn ở độ cao `H_tram` so với đáy và nhìn xuống. Nước dâng thì `D_loc` nhỏ đi và `H_nuoc` lớn lên. Kết quả âm thì lấy bằng 0.

Khi hạ mức, cảnh báo có độ trễ `HYST_CM = 0.5 cm` để không đổi qua đổi lại liên tục khi nước dao động quanh ngưỡng:

| Mức | Lên mức khi | Hạ mức khi |
|---|---|---|
| ĐỎ (`red`) | `H_nuoc ≥ 8.0` | `H_nuoc < 7.5` |
| VÀNG (`yellow`) | `H_nuoc ≥ 4.0` | `H_nuoc < 3.5` |
| BÌNH THƯỜNG (`normal`) | còn lại | |

Ví dụ: `D_loc = 24.6 cm` thì `H_nuoc = 30 − 24.6 = 5.4 cm`, mức VÀNG.

### 3.4. Lưu lượng từ YF-S201 (`flow.cpp`)

Nước chảy qua thì cảm biến phát xung, và mỗi xung làm ngắt tăng biến đếm thêm 1. Mỗi chu kỳ code lấy số xung rồi đặt lại về 0.

```
f = số_xung × 1000 / elapsed_ms     (Hz, số xung mỗi giây)

Q = f / 7.5                         (lít/phút, 7.5 = FLOW_K)

Tổng lít += (Q / 60) × (elapsed_ms / 1000)
```

- `f = 7.5 × Q` là đặc tính của YF-S201: 7.5 xung/giây ứng với 1 lít/phút.
- Code chia cho `elapsed_ms` đo thực tế thay vì giả định đúng 1000 ms, vì đo siêu âm và gửi HTTP có thể làm chu kỳ dài hơn 1 giây.
- `Q / 60` đổi lít/phút sang lít/giây, nhân với số giây của chu kỳ thì ra số lít chảy qua trong chu kỳ đó. Gộp lại, 450 xung ứng với 1 lít.

Ví dụ: 30 xung trong 1000 ms thì `f = 30 Hz`, `Q = 30 / 7.5 = 4.00 lít/phút`, và tổng cộng thêm `4 / 60 × 1 ≈ 0.067 lít`.

### 3.5. Độ ẩm đất (`soil.cpp`)

Code đọc ADC 10 lần, cách nhau 2 ms, rồi lấy trung bình để giảm nhiễu. ADC của ESP32 cho giá trị từ 0 tới 4095, và với module này đất càng khô thì ADC càng cao.

```
W = (ADC_kho − ADC) / (ADC_kho − ADC_uot) × 100      (%)
```

Kết quả được giới hạn trong khoảng 0 tới 100.

Ví dụ với `ADC_kho = 3500`, `ADC_uot = 1400`, đọc được `ADC = 2450`: `W = (3500 − 2450) / (3500 − 1400) × 100 = 50%`.

Hai giá trị hiệu chuẩn trong `config.h` chỉ là ví dụ. Để đầu dò ngoài không khí, đọc số `ADC` trên Serial và ghi vào `ADC_KHO`. Sau đó nhúng đầu dò vào nước, đọc số `ADC` và ghi vào `ADC_UOT`.

### 3.6. Cảnh báo lưu lượng, độ ẩm đất và lũ quét (`alert.cpp`)

Lưu lượng và độ ẩm đất cũng có mức vàng/đỏ, dùng chung hàm `levelFromValue()` với mực nước (cùng kiểu độ trễ khi hạ mức). Lưu lượng được lấy trung bình động 5 mẫu (`FLOW_MA_N`, mỗi mẫu 1 giây) trước khi so với mốc, vì lưu lượng đo từng giây nhảy khá mạnh.

| | Vàng | Đỏ | Độ trễ hạ mức |
|---|---|---|---|
| Lưu lượng Q (trung bình 5 giây) | `Q ≥ 10` L/phút | `Q ≥ 20` L/phút | 1 L/phút |
| Độ ẩm đất W | `W ≥ 70` % | `W ≥ 90` % | 3 % |

Các mốc này là giá trị đề xuất ban đầu, chưa được hiệu chuẩn tại chỗ đặt trạm. Hãy đổi trong `config.h` cho hợp với lưu lượng bình thường của bạn.

**Lũ quét** ghép hai mức trên (`flashFloodLevel()`):

| Mức lũ quét | Điều kiện |
|---|---|
| ĐỎ | Lưu lượng ≥ vàng **và** độ ẩm đất ≥ vàng cùng lúc: nước chảy mạnh trên nền đất đã no nước |
| VÀNG | Chỉ một trong hai đạt mức đỏ: nước chảy rất mạnh, hoặc đất đã bão hòa |
| BÌNH THƯỜNG | Còn lại |

**Mức tổng hợp** (`level` trong JSON, hiện ở banner dashboard) là mức cao hơn giữa mực nước và lũ quét. Lưu lượng hoặc độ ẩm chỉ đạt vàng một mình thì chỉ đổi màu thẻ riêng, không đổi banner, để tránh báo động giả khi có mưa rào. Muốn ghép khác thì sửa hàm `flashFloodLevel()` (và `flashOf()` trong `index.html`, `flashFloodLevel()` trong `generate-data.js` cho khớp).

ESP32 gửi `alert` ngay khi **bất kỳ** mức nào trong bốn mức (mực nước, lưu lượng, độ ẩm, lũ quét) đổi.

## 4. Web server

Thư mục `web/` chỉ dùng module có sẵn của Node.js nên không cần `npm install`. Cài Node.js bản LTS từ https://nodejs.org rồi chạy:

```powershell
cd web
node server.js
```

Dashboard ở `http://localhost:3000`, tự làm mới mỗi 5 giây.

| File | Nhiệm vụ |
|---|---|
| `server.js` | Nhận `POST /flood`, ghi từng bản tin vào `data.jsonl`, trả dữ liệu cho dashboard |
| `index.html` | Dashboard gồm 4 phần: (1) dữ liệu bây giờ, (2) dự báo số liệu, (3) mực nước theo thời gian, (4) bản tin trong 1 tuần qua theo ngày. Ô chọn ngày cạnh tiêu đề (hoặc bấm một dòng trong bảng 4) cho xem lại một ngày trong tuần: phần 1 hiện bản tin cuối ngày, phần 3 hiện mực nước cả ngày, phần 2 (dự báo) bị ẩn |
| `forecast.js`, `generate-data.js`, `train.js`, `model.json` | Dự báo mực nước, lưu lượng, độ ẩm đất (xem mục 5) |

Các đường dẫn server có:

| Đường dẫn | Dùng cho |
|---|---|
| `POST /flood` | ESP32 gửi bản tin. Body sai định dạng bị trả mã 400 |
| `GET /` | Dashboard |
| `GET /api/history?limit=N` | N bản tin gần nhất (mặc định 300) |
| `GET /api/daily?days=N` | Trung bình theo ngày của N ngày gần nhất (mặc định 7, tối đa 31, ngày cũ nhất trước), ví dụ `{"date":"2026-11-12","n":14623,"soil_pct":68.4,"h_nuoc_cm":1.09,"flow_lpm":0.43,"total_l":501.8}`. Ngày không có dữ liệu thì `n = 0` và các số là `null` |
| `GET /api/day?date=YYYY-MM-DD` | Xem lại một ngày: `last` là bản tin cuối ngày (hoặc `null` nếu không có dữ liệu) và `points` là mực nước trung bình mỗi 2 phút trong ngày |
| `GET /api/forecast` | Dự báo mực nước, lưu lượng, độ ẩm đất, ví dụ `{"horizons_min":[5,10],"h_cm":[5.8,7.7],"flow_lpm":[8.1,12.0],"soil_pct":[73.0,75.0]}`. Cả ba là `null` khi chưa có model hoặc chưa đủ 5 phút dữ liệu liên tục |

Server giữ 2000 bản tin gần nhất trong bộ nhớ (dùng cho biểu đồ và dự báo), còn `data.jsonl` thì lưu tất cả và được nạp lại khi khởi động. Bảng "1 tuần qua" tính trên toàn bộ `data.jsonl`: độ ẩm, mực nước và lưu lượng trung bình lấy từ các bản tin định kỳ trong ngày (theo giờ của máy chạy server); tổng lưu lượng là lượng nước chảy qua trong ngày, cộng các phần tăng của `total_l` (trạm reset thì `total_l` về 0 và được tính lại từ đầu). Hai ngưỡng `YELLOW_CM` / `RED_CM` ở đầu đoạn script trong `index.html` phải khớp với `config.h`.

### 4.1. Request ESP32 gửi

```
POST http://<IP-server>:3000/flood
Content-Type: application/json
```

Đường dẫn và cổng lấy từ `SERVER_URL` trong `config.h`.

Body:

```json
{
  "event": "alert",
  "level": "yellow",
  "water_level": "yellow",
  "flow_level": "normal",
  "soil_level": "normal",
  "flash_level": "normal",
  "h_nuoc_cm": 5.4,
  "d_loc_cm": 24.6,
  "h_tram_cm": 30.0,
  "flow_lpm": 4.00,
  "flow_avg_lpm": 4.00,
  "total_l": 0.067,
  "soil_pct": 50.0
}
```

| Trường | Kiểu | Ý nghĩa |
|---|---|---|
| `event` | chuỗi | `"alert"`: mức cảnh báo vừa đổi. `"report"`: báo cáo định kỳ |
| `level` | chuỗi | Mức tổng hợp: `"normal"`, `"yellow"` hoặc `"red"` (mức cao hơn giữa mực nước và lũ quét, mục 3.6) |
| `water_level`, `flow_level`, `soil_level`, `flash_level` | chuỗi | Mức theo mực nước, lưu lượng, độ ẩm đất và lũ quét. Tùy chọn: bản tin cũ không có |
| `flow_avg_lpm` | số | Lưu lượng trung bình 5 giây dùng để cảnh báo. Tùy chọn |
| `h_nuoc_cm` | số | Mực nước (cm), luôn ≥ 0 |
| `d_loc_cm` | số | Khoảng cách đã lọc từ cảm biến tới mặt nước (cm) |
| `h_tram_cm` | số | Chiều cao lắp cảm biến (cm), hằng số cấu hình |
| `flow_lpm` | số | Lưu lượng (lít/phút) của 1 giây gần nhất |
| `total_l` | số | Tổng số lít từ lúc ESP32 bật nguồn |
| `soil_pct` | số | Độ ẩm đất (%), từ 0 tới 100 |

### 4.2. Khi nào ESP32 gửi

| `event` | Thời điểm |
|---|---|
| `report` | Lần đầu khoảng 5 giây sau khi khởi động (lúc bộ lọc đủ mẫu), sau đó mỗi 5 giây (`REPORT_INTERVAL_MS`) |
| `alert` | Ngay khi mức cảnh báo nào đó đổi (mực nước, lưu lượng, độ ẩm đất hoặc lũ quét), cả lúc tăng lẫn lúc hạ (ví dụ `yellow` về `normal`) |

Server có thể nhận `alert` và `report` liền nhau trong cùng một giây.

### 4.3. Server cần trả về gì

ESP32 coi mã 2xx (ví dụ `200`) là gửi thành công và không đọc body trả về. Server phải trả lời trong vòng 5 giây; quá thời gian đó ESP32 coi là lỗi, và trong lúc chờ thì ESP32 tạm ngừng đo.

### 4.4. Những điều cần biết về dữ liệu

- JSON không có thời gian. ESP32 không có đồng hồ thực nên server tự gắn thời gian lúc nhận (`received_at`).
- JSON không có mã thiết bị. Nếu sau này có nhiều trạm thì cần thêm trường `device_id` trong `uplink.cpp`.
- ESP32 không gửi lại khi lỗi. Nếu mất WiFi hoặc server không trả lời, bản tin đó bị bỏ. Một `alert` bị mất thì server chỉ biết mức mới qua `report` kế tiếp (tối đa 5 giây sau), nên hãy lấy `level` từ mọi bản tin chứ đừng chỉ dựa vào `alert`.
- `total_l` về 0 khi ESP32 khởi động lại. Giá trị nhỏ hơn lần trước nghĩa là trạm vừa reset.
- `flow_lpm` là giá trị tức thời của 1 giây ngay trước khi gửi. Muốn biết lượng nước giữa hai bản tin thì lấy hiệu `total_l`.
- Kết nối chỉ dùng HTTP, không có HTTPS và không có xác thực.
- Dashboard báo mất kết nối khi quá 20 giây không nhận được bản tin nào, tức là đã lỡ 3 đến 4 bản tin.

### 4.5. Điều kiện mạng

- Máy chạy server và ESP32 phải cùng mạng WiFi/LAN, hoặc server có IP công khai.
- Server lắng nghe trên `0.0.0.0` chứ không chỉ `localhost`, nên các máy khác trong mạng gửi tới được.
- Mở cổng 3000 trên tường lửa của máy chạy server. Lần đầu chạy, Windows hỏi thì chọn Allow.
- Lấy IP của máy chạy server bằng `ipconfig` (dòng IPv4 của card WiFi) rồi ghi vào `SERVER_URL`. Nên đặt IP tĩnh cho máy này, vì IP đổi thì phải sửa `config.h` và nạp lại ESP32.

### 4.6. Thử server khi chưa có ESP32

Gửi một bản tin giả bằng PowerShell:

```powershell
$body = '{"event":"alert","level":"yellow","h_nuoc_cm":5.4,"d_loc_cm":24.6,"h_tram_cm":30.0,"flow_lpm":4.00,"total_l":0.067,"soil_pct":50.0}'
Invoke-RestMethod -Uri http://localhost:3000/flood -Method Post -ContentType 'application/json' -Body $body
```

Khi chạy với ESP32 thật, dòng `POST report -> 200` trên Serial Monitor cho biết server đã nhận được.

## 5. Dự báo mực nước, lưu lượng và độ ẩm đất

Dashboard dự báo **mực nước, lưu lượng và độ ẩm đất** sau 5 và 10 phút. Mực nước dự báo được vẽ thành đường nét đứt nối tiếp biểu đồ; lưu lượng và độ ẩm dự báo hiện ở hai thẻ riêng. Nếu mức tổng hợp tính từ giá trị dự báo (mực nước, hoặc lũ quét ghép từ lưu lượng và độ ẩm, theo mục 3.6) cao hơn mức hiện tại, dashboard hiện thêm dòng như "Dự báo: có thể lên CẢNH BÁO ĐỎ trong khoảng 10 phút nữa (lũ quét: 12.0 L/phút, đất 75%)". Trạm mất kết nối thì dashboard không hiện dự báo.

Mô hình được huấn luyện sẵn và lưu trong `web/model.json`, nên chạy `node server.js` là có dự báo. Các bước dưới đây chỉ cần khi muốn huấn luyện lại.

### 5.1. Dữ liệu giả lập (`generate-data.js`)

Nhóm chưa có dữ liệu thật, nên script này mô phỏng trạm theo chuỗi mưa → đất ẩm dần → nước chảy qua ống (YF-S201) → bể dâng:

- Mỗi cơn mưa dài 10 đến 60 phút, cường độ đỉnh 5 đến 60 mm/h, hai cơn cách nhau 1 đến 5 giờ. Khoảng 1 cơn trong 10 là bão lớn (80 đến 150 mm/h): chỉ cơn như vậy mới đưa lưu lượng qua mốc đỏ 20 L/phút, nên dữ liệu huấn luyện mới có sự kiện lũ quét.
- Đất càng ướt thì lượng nước chảy tràn càng nhiều. Dòng chảy tới ống chậm khoảng 2 phút so với mưa.
- Bể nhận nước theo lưu lượng và thoát nước theo mực nước, với hằng số thời gian 15 phút.
- Số đo có cộng nhiễu như cảm biến thật. Mức cảnh báo (mực nước, lưu lượng, độ ẩm, lũ quét, tổng hợp) dùng cùng ngưỡng và độ trễ với firmware, và có bản tin `alert` mỗi khi bất kỳ mức nào đổi.
- Script in thống kê số bản tin ở từng mức để kiểm tra dữ liệu có đủ sự kiện cho mô hình học.

```powershell
cd web
node generate-data.js          # 30 ngày, seed 1 -> synthetic.jsonl (khoảng 90 MB, không đưa lên git)
node generate-data.js 3 42     # 3 ngày, seed 42
```

Mỗi bản tin có cùng định dạng với `data.jsonl` mà server ghi, cách nhau 5 giây như ESP32. Cùng seed thì ra cùng bộ dữ liệu.

### 5.2. Mô hình (`forecast.js`)

Mô hình là hồi quy tuyến tính, dự báo độ thay đổi của **mực nước, lưu lượng và độ ẩm đất** sau 5 và 10 phút (3 đại lượng × 2 mốc = 6 đầu ra). Đầu vào là 10 đặc trưng lấy từ 5 phút gần nhất:

- mực nước, lưu lượng và độ ẩm đất hiện tại;
- mực nước đã dâng bao nhiêu trong 30 giây, 1, 2 và 5 phút qua;
- lưu lượng đã thay đổi bao nhiêu trong 1 và 5 phút qua;
- độ ẩm đất đã thay đổi bao nhiêu trong 5 phút qua.

Kết quả dự báo được giới hạn trong khoảng hợp lý: mực nước từ 0 tới `h_tram_cm`, lưu lượng không âm, độ ẩm từ 0 tới 100%. `model.json` ghi cả danh sách đại lượng; file cũ chỉ dự báo mực nước sẽ bị server bỏ qua và `train.js --finetune` từ chối, cần pre-train lại.

Mô hình học bằng gradient descent để có thể huấn luyện tiếp (fine-tune) trên dữ liệu mới từ chính các trọng số cũ. `train.js` (huấn luyện) và `server.js` (dự báo) dùng chung file này, nên đặc trưng lúc huấn luyện và lúc dự báo luôn khớp nhau.

### 5.3. Huấn luyện (`train.js`)

```powershell
node train.js                          # pre-train từ đầu trên synthetic.jsonl
node train.js --finetune data.jsonl    # fine-tune model.json trên dữ liệu trạm thật
```

Script chia dữ liệu theo thời gian: 80% đầu để huấn luyện và 20% cuối để kiểm tra. Sai số của từng đại lượng ở từng mốc được so với cách đoán đơn giản nhất là giữ nguyên giá trị hiện tại. Nếu có đầu ra nào không tốt hơn cách đó thì script không ghi đè `model.json` và nói rõ đầu ra nào. Huấn luyện xong phải khởi động lại `server.js` để nạp model mới.

`model.json` hiện tại được huấn luyện 1500 vòng trên 30 ngày giả lập (khoảng 518.000 mẫu, seed 1), mất khoảng 5 phút.

Sai số tuyệt đối trung bình khi thử trên 7 ngày giả lập mà mô hình chưa từng thấy (seed 99). "Đang tăng" là những thời điểm mà 10 phút sau giá trị cao hơn ít nhất 1 cm (mực nước), 2 L/phút (lưu lượng) hoặc 5% (độ ẩm), tức lúc dự báo có ích nhất:

| Đại lượng, mốc | Mô hình, mọi lúc | Giữ nguyên, mọi lúc | Mô hình, đang tăng | Giữ nguyên, đang tăng |
|---|---|---|---|---|
| Mực nước +5 phút | 0.15 cm | 0.39 cm | 0.29 cm | 1.70 cm |
| Mực nước +10 phút | 0.29 cm | 0.64 cm | 1.26 cm | 3.40 cm |
| Lưu lượng +5 phút | 0.25 L/phút | 0.35 L/phút | 1.42 L/phút | 2.67 L/phút |
| Lưu lượng +10 phút | 0.56 L/phút | 0.63 L/phút | 4.46 L/phút | 5.48 L/phút |
| Độ ẩm đất +5 phút | 1.06 % | 1.32 % | 3.23 % | 6.47 % |
| Độ ẩm đất +10 phút | 1.86 % | 2.39 % | 7.87 % | 12.93 % |

Lưu lượng là đại lượng dự báo kém nhất: mô hình chỉ hơn cách giữ nguyên một chút ở mốc +10 phút, và khi lưu lượng đang tăng nhanh thì sai số trung bình vẫn tới 4.5 L/phút. Lý do là trạm chưa đo mưa nên mô hình không biết trước nước sắp đổ về (xem mục 5.4).

Lệnh `--finetune` đã chạy thử trên model mới và hoạt động, nhưng chưa đo được lợi ích của nó: thử trên dữ liệu giả lập cùng bộ sinh thì sai số gần như không đổi, như dự kiến. Lợi ích thật chỉ kiểm chứng được trên dữ liệu `data.jsonl` của trạm thật.

### 5.4. Giới hạn

- Mô hình chỉ học được những gì có trong dữ liệu. Dữ liệu giả lập đi theo giả định vật lý của `generate-data.js`, nên dự báo cho trạm thật chỉ đáng tin sau khi đã fine-tune trên `data.jsonl` của trạm.
- Fine-tune cần ít nhất 100 mẫu, tức khoảng 25 phút dữ liệu liên tục (5 phút nhìn lại, 10 phút nhìn tới để có đáp án, và 100 mẫu cách nhau 5 giây). Muốn mô hình học được lúc nước dâng thì trong dữ liệu phải có những lần nước dâng thật, ví dụ đổ nước vào bể với nhiều tốc độ khác nhau, và cũng cần những lần lưu lượng và độ ẩm đất thay đổi thật.
- Mốc cảnh báo lưu lượng (10/20 L/phút) và độ ẩm (70/90%) là phỏng đoán, chưa hiệu chuẩn; dữ liệu giả lập chỉ có đủ sự kiện lũ quét vì bộ sinh được chỉnh cho có bão lớn.
- Trạm chưa đo mưa. Thêm cảm biến mưa hoặc lấy lượng mưa từ API thời tiết sẽ cho mô hình biết trước nước sắp dâng.
- Mô hình tuyến tính đủ cho một bể nước đơn giản. Nếu sai số trên dữ liệu thật vẫn lớn thì có thể thử mạng nơ-ron nhỏ (MLP) hoặc LightGBM với cùng bộ đặc trưng.
