# Pico 2 Active Crossover

Pico 2 (RP2350) làm card âm thanh USB, tách tín hiệu thành 3 dải (LOW / MID / HIGH) bằng
crossover Linkwitz-Riley bậc 4, rồi xuất ra 3 đường I2S để nối 3 DAC.

> **Trạng thái: chưa kiểm chứng.** Mã chưa được biên dịch hay chạy thử trên phần cứng khi tạo repo này.
> Lần đầu đẩy lên GitHub, hãy xem tab **Actions**: nếu có lỗi biên dịch (nhiều khả năng liên quan đến
> tên macro/hàm của TinyUSB theo phiên bản), copy nội dung lỗi để sửa.

## Cách hoạt động

```
PC ─USB (UAC2, 48 kHz, 24-bit)→ TinyUSB → ring buffer → DSP (core 1) → 3 vòng đệm → DMA → 3 SM PIO → 3 DAC
```

- **Core 0:** TinyUSB, điều khiển volume/mute, tính giá trị feedback cho host.
- **Core 1:** crossover 300 Hz / 3 kHz, xử lý theo khối 32 mẫu.
- **PIO:** 3 state machine chạy cùng một chương trình I2S, dùng chung BCLK/LRCLK, mỗi SM một chân DATA.
- **DMA:** mỗi SM hai kênh DMA chain vòng quanh ring buffer, không cần ngắt.

## Chân GPIO

| Tín hiệu | GPIO |
|---|---|
| BCLK | 10 |
| LRCLK | 11 |
| DATA LOW | 12 |
| DATA MID | 13 |
| DATA HIGH | 14 |

Đổi trong `i2s_out.h`. Mỗi đường mang một dải stereo (L/R). Chưa có MCLK, nếu DAC cần thì thêm sau.

## Biên dịch

### Cách 1: GitHub Actions (không cần cài gì)
Đẩy code lên GitHub, vào tab **Actions**, mở lần chạy mới nhất, tải artifact `xover-uf2`.

### Cách 2: trên máy
1. Cài Pico SDK **2.1.x** (cách dễ nhất: extension *Raspberry Pi Pico* của VS Code).
2. Đặt `PICO_SDK_PATH`, rồi:
   ```
   cmake -S . -B build
   cmake --build build -j
   ```
3. Giữ nút **BOOTSEL** khi cắm Pico 2 vào USB, kéo `build/xover.uf2` vào ổ đĩa `RP2350`.

## Cấu trúc

| File | Vai trò |
|---|---|
| `main.c` | Clock hệ thống, ring USB, vòng DSP trên core 1, feedback |
| `i2s3.pio` | Chương trình PIO I2S master 32-bit |
| `i2s_out.c/.h` | Khởi tạo PIO + DMA, vòng đệm đầu ra |
| `crossover.h` | Biquad float + crossover LR4 3 dải (có allpass bù pha cho dải LOW) |
| `usb_descriptors.c/.h` | Descriptor UAC2 (loa stereo, 24-bit, feedback endpoint) |
| `usb_audio.c` | Callback điều khiển: clock, volume, mute, alt setting |
| `tusb_config.h` | Cấu hình TinyUSB |
| `app.h` | Hằng số và biến dùng chung |

## Ghi chú kỹ thuật

- **Clock:** hệ thống chạy **153,6 MHz** (VCO 1536 MHz / 5 / 2), hơi cao hơn mức 150 MHz mặc định nên
  firmware đặt áp lõi 1,15 V. Với 153,6 MHz, bộ chia PIO cho 48 kHz là đúng 25,0 (jitter thấp).
- **Chỉ hỗ trợ 48 kHz.** Muốn thêm 44,1 kHz / 96 kHz cần cấu hình lại PLL theo từng tần số.
- **Feedback USB:** hệ số `err * 8.0f` trong `fb_task()` là giá trị khởi điểm, cần chỉnh khi thử với PC thật.
  Firmware dùng `tud_audio_n_fb_set()` với method `DISABLED`, hãy đối chiếu với phiên bản TinyUSB trong SDK.
- **Volume:** master từ Windows/macOS, dải -60 dB đến 0 dB, áp trước crossover.
- **VID/PID:** đang dùng `0xCafe:0x4010` (thử nghiệm). Không dùng khi phát hành sản phẩm.
- **Độ trễ:** vài ms ở DSP/DMA, cộng thêm đệm USB mục tiêu 512 frame (~10,7 ms).
- **Chưa có:** delay căn thời gian giữa các loa, PEQ từng dải, limiter, đảo cực, đổi tần số cắt khi chạy.

## Thử nghiệm theo thứ tự gợi ý

1. Đo BCLK/LRCLK/DATA bằng logic analyzer (tín hiệu im lặng vẫn có clock).
2. Phát nhạc từ PC, kiểm tra từng đường I2S có dữ liệu đúng dải.
3. Theo dõi mức đệm USB (`usb_w - usb_r`) xem có trôi hay dao động không để chỉnh feedback.
