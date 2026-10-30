// Pico 2 (RP2350): USB Audio -> crossover 3 dải -> 3 đường I2S
// Core 0: TinyUSB + feedback.  Core 1: DSP + cấp dữ liệu cho DMA.
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/sync.h"
#include "tusb.h"
#include "app.h"
#include "i2s_out.h"
#include "crossover.h"

#define F1_HZ       300.0f
#define F2_HZ       3000.0f

#define BLOCK       32                 // frame mỗi lần DSP (0.67 ms); RING_FRAMES chia hết
#define AHEAD       160                // mức "viết trước" DMA (frame, ~3.3 ms)

#define USB_RING    2048               // frame, lũy thừa của 2
#define USB_TARGET  512                // mức đệm mục tiêu (frame)

volatile float g_gain  = 1.0f;
volatile bool  g_flush = false;

// ---- Ring SPSC: core 0 ghi (producer), core 1 đọc (consumer) ----
static int32_t           usb_ring[USB_RING * 2];
static volatile uint32_t usb_w, usb_r;

static void usb_push(const int32_t *s, uint32_t frames) {
    uint32_t w = usb_w;
    for (uint32_t i = 0; i < frames; i++) {
        if ((w - usb_r) >= USB_RING) break;               // đầy: bỏ mẫu
        usb_ring[(w & (USB_RING - 1)) * 2 + 0] = s[i * 2 + 0];
        usb_ring[(w & (USB_RING - 1)) * 2 + 1] = s[i * 2 + 1];
        w++;
    }
    __dmb();
    usb_w = w;
}

// ---- DSP trên core 1 ----
static inline uint32_t to_i32(float x) {
    if (x >  0.9999999f) x =  0.9999999f;
    if (x < -1.0f)       x = -1.0f;
    return (uint32_t)(int32_t)(x * 2147483648.0f);
}

static void __not_in_flash_func(dsp_core)(void) {
    static xover_t xl, xr;
    xover_init(&xl, F1_HZ, F2_HZ, FS);
    xover_init(&xr, F1_HZ, F2_HZ, FS);

    bool primed = false;
    uint32_t wr = (i2s_out_play_pos() + 192) & (RING_FRAMES - 1);
    const float k = 1.0f / 2147483648.0f;

    for (;;) {
        uint32_t rp    = i2s_out_play_pos();
        uint32_t ahead = (wr - rp) & (RING_FRAMES - 1);
        if (ahead > RING_FRAMES / 2) { wr = (rp + 192) & (RING_FRAMES - 1); continue; }  // bị DMA vượt: đồng bộ lại
        if (ahead > AHEAD) { tight_loop_contents(); continue; }

        if (g_flush) { usb_r = usb_w; primed = false; g_flush = false; }

        uint32_t fill = usb_w - usb_r;
        if (!primed && fill >= USB_TARGET) primed = true;
        if (primed && fill < BLOCK)        primed = false;     // underrun: im lặng, đệm lại

        float g = g_gain;
        for (int i = 0; i < BLOCK; i++) {
            float l = 0.0f, r = 0.0f;
            if (primed) {
                uint32_t idx = (usb_r + i) & (USB_RING - 1);
                l = usb_ring[idx * 2 + 0] * k * g;
                r = usb_ring[idx * 2 + 1] * k * g;
            }
            float ol[3], orr[3];
            xover_run(&xl, l, ol);
            xover_run(&xr, r, orr);
            uint32_t pos = (wr + i) & (RING_FRAMES - 1);
            for (int b = 0; b < I2S_BANDS; b++) {
                i2s_ring[b][pos * 2 + 0] = to_i32(ol[b]);
                i2s_ring[b][pos * 2 + 1] = to_i32(orr[b]);
            }
        }
        if (primed) { __dmb(); usb_r += BLOCK; }
        wr = (wr + BLOCK) & (RING_FRAMES - 1);
    }
}

// ---- USB: đọc dữ liệu host gửi (24-bit nằm trong slot 32-bit, căn trái) ----
static void audio_rx_task(void) {
    uint16_t avail = tud_audio_available();
    while (avail >= 8) {
        int32_t tmp[64];
        uint16_t n = avail < sizeof tmp ? avail : sizeof tmp;
        n &= ~7u;                                         // bội số của 1 frame (8 byte)
        n = tud_audio_read(tmp, n);
        if (n == 0) break;
        usb_push(tmp, n / 8);
        avail = tud_audio_available();
    }
}

// ---- Feedback endpoint (Full-Speed: định dạng 10.14, mẫu/ms) ----
// Pico là master clock. Nếu đệm USB đầy lên -> báo host gửi chậm lại, cạn -> gửi nhanh.
static void fb_task(void) {
    static absolute_time_t next;
    static float fill_avg = USB_TARGET;
    if (!time_reached(next)) return;
    next = delayed_by_ms(get_absolute_time(), 1);

    fill_avg += ((float)(usb_w - usb_r) - fill_avg) * (1.0f / 64.0f);
    float err = (float)USB_TARGET - fill_avg;             // dương = đệm đang cạn
    int32_t nom = (FS / 1000) * (1 << 14);
    int32_t fb  = nom + (int32_t)(err * 8.0f);
    int32_t lim = 1 << 13;                                // ±0.5 mẫu/ms
    if (fb > nom + lim) fb = nom + lim;
    if (fb < nom - lim) fb = nom - lim;
    tud_audio_n_fb_set(0, (uint32_t)fb);
}

int main(void) {
    // 153.6 MHz = 1536 MHz / 5 / 2 -> chia PIO đúng 25.0 cho 48 kHz (jitter thấp)
    vreg_set_voltage(VREG_VOLTAGE_1_15);
    sleep_ms(2);
    set_sys_clock_pll(1536000000u, 5, 2);

    i2s_out_init((float)clock_get_hz(clk_sys), (float)FS);
    i2s_out_start();                                      // phát im lặng ngay từ đầu
    multicore_launch_core1(dsp_core);

    tusb_init();
    for (;;) {
        tud_task();
        audio_rx_task();
        fb_task();
    }
}
