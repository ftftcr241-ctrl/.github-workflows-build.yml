#include <string.h>
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "i2s_out.h"
#include "i2s3.pio.h"

// Moi band 4096 byte, can theo 4096 de DMA ring wrap doc dung
uint32_t __attribute__((aligned(RING_FRAMES * 8))) i2s_ring[I2S_BANDS][RING_FRAMES * 2];

static PIO pio = pio0;
static int dma_a[I2S_BANDS], dma_b[I2S_BANDS];

// Hai kenh DMA moi SM, chain vong A -> B -> A. Moi kenh chuyen dung 1 vong ring
// nen dia chi doc tu wrap ve dau ring, khong can ngat va khong can CPU.
static void cfg_channel(int ch, int next, int sm) {
    dma_channel_config c = dma_channel_get_default_config(ch);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_ring(&c, false, RING_BYTES_LOG2);   // wrap phia doc
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    channel_config_set_chain_to(&c, next);
    dma_channel_configure(ch, &c, &pio->txf[sm], i2s_ring[sm], RING_FRAMES * 2, false);
}

void i2s_out_init(float sys_hz, float fs) {
    memset(i2s_ring, 0, sizeof i2s_ring);

    uint offset = pio_add_program(pio, &i2s3_out_program);
    float div = sys_hz / (fs * 128.0f);        // 153.6e6 / 6.144e6 = 25.0 (nguyen)

    pio_gpio_init(pio, I2S_BCLK_PIN);
    pio_gpio_init(pio, I2S_BCLK_PIN + 1);

    for (int sm = 0; sm < I2S_BANDS; sm++) {
        pio_sm_claim(pio, sm);
        pio_gpio_init(pio, I2S_DATA_PIN0 + sm);
        // Ca 3 SM cung side-set len BCLK/LRCLK (gia tri y het nhau). Phai bat output
        // enable o MOI SM, vi SM so cao nhat ghi chan thi OE cua no moi co hieu luc.
        pio_sm_set_consecutive_pindirs(pio, sm, I2S_BCLK_PIN, 2, true);
        pio_sm_set_consecutive_pindirs(pio, sm, I2S_DATA_PIN0 + sm, 1, true);

        pio_sm_config c = i2s3_out_program_get_default_config(offset);
        sm_config_set_out_pins(&c, I2S_DATA_PIN0 + sm, 1);
        sm_config_set_sideset_pins(&c, I2S_BCLK_PIN);
        sm_config_set_out_shift(&c, false, true, 32);   // MSB first, autopull 32 bit
        sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
        sm_config_set_clkdiv(&c, div);
        pio_sm_init(pio, sm, offset + i2s3_out_offset_entry_point, &c);

        dma_a[sm] = dma_claim_unused_channel(true);
        dma_b[sm] = dma_claim_unused_channel(true);
    }
    for (int sm = 0; sm < I2S_BANDS; sm++) {
        cfg_channel(dma_a[sm], dma_b[sm], sm);
        cfg_channel(dma_b[sm], dma_a[sm], sm);
    }
}

void i2s_out_start(void) {
    uint32_t mask = 0;
    for (int sm = 0; sm < I2S_BANDS; sm++) mask |= 1u << dma_a[sm];
    dma_start_channel_mask(mask);                       // DMA do day FIFO truoc
    for (int sm = 0; sm < I2S_BANDS; sm++)
        while (!pio_sm_is_tx_fifo_full(pio, sm)) tight_loop_contents();
    pio_enable_sm_mask_in_sync(pio, (1u << I2S_BANDS) - 1);   // 3 SM bat dau cung chu ky
}

uint32_t i2s_out_play_pos(void) {
    uint32_t ra = dma_channel_is_busy(dma_a[0]) ? dma_hw->ch[dma_a[0]].read_addr
                                                : dma_hw->ch[dma_b[0]].read_addr;
    return ((ra - (uint32_t)i2s_ring[0]) >> 3) & (RING_FRAMES - 1);
}
