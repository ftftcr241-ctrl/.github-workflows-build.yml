#pragma once
#include <stdint.h>

// Chan GPIO: BCLK = 10, LRCLK = 11 (BCLK + 1), DATA0..2 = 12, 13, 14
#define I2S_BCLK_PIN   10
#define I2S_DATA_PIN0  12
#define I2S_BANDS      3

#define RING_FRAMES    512            // luy thua cua 2; 512 frame = 4096 byte = 2^12
#define RING_BYTES_LOG2 12

// i2s_ring[band][frame*2 + ch]  (ch 0 = L, 1 = R), du lieu int32 can trai
extern uint32_t i2s_ring[I2S_BANDS][RING_FRAMES * 2];

void     i2s_out_init(float sys_hz, float fs);
void     i2s_out_start(void);
uint32_t i2s_out_play_pos(void);      // frame (0..RING_FRAMES-1) DMA dang doc
