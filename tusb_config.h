#pragma once
#include "app.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif

#define CFG_TUSB_OS                 OPT_OS_NONE
#define CFG_TUSB_DEBUG              0

#define CFG_TUD_ENABLED             1
#define CFG_TUD_MAX_SPEED           OPT_MODE_FULL_SPEED
#define CFG_TUSB_RHPORT0_MODE       (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define CFG_TUD_ENDPOINT0_SIZE      64

// ---- Chỉ dùng Audio class ----
#define CFG_TUD_CDC                 0
#define CFG_TUD_MSC                 0
#define CFG_TUD_HID                 0
#define CFG_TUD_MIDI                0
#define CFG_TUD_VENDOR              0
#define CFG_TUD_AUDIO               1

// ---- Audio: UAC2, stereo, 24-bit trong slot 4 byte, host -> thiết bị, có feedback ----
#define CFG_TUD_AUDIO_ENABLE_EP_IN        0
#define CFG_TUD_AUDIO_ENABLE_EP_OUT       1
#define CFG_TUD_AUDIO_ENABLE_FEEDBACK_EP  1
#define CFG_TUD_AUDIO_ENABLE_ENCODING     0
#define CFG_TUD_AUDIO_ENABLE_DECODING     0

// Độ dài descriptor audio (cộng các thành phần; macro *_LEN nằm trong audio_device.h)
#define XOVER_AUDIO_DESC_LEN ( TUD_AUDIO_DESC_IAD_LEN \
        + TUD_AUDIO_DESC_STD_AC_LEN \
        + TUD_AUDIO_DESC_CS_AC_LEN \
        + TUD_AUDIO_DESC_CLK_SRC_LEN \
        + TUD_AUDIO_DESC_INPUT_TERM_LEN \
        + TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL_LEN \
        + TUD_AUDIO_DESC_OUTPUT_TERM_LEN \
        + TUD_AUDIO_DESC_STD_AS_INT_LEN \
        + TUD_AUDIO_DESC_STD_AS_INT_LEN \
        + TUD_AUDIO_DESC_CS_AS_INT_LEN \
        + TUD_AUDIO_DESC_TYPE_I_FORMAT_LEN \
        + TUD_AUDIO_DESC_STD_AS_ISO_EP_LEN \
        + TUD_AUDIO_DESC_CS_AS_ISO_EP_LEN \
        + TUD_AUDIO_DESC_STD_AS_ISO_FB_EP_LEN )

#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN                XOVER_AUDIO_DESC_LEN
#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT                1
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ             64
#define CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_RX   4
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX           2

// Gói isochronous lớn nhất: (48 + 1 mẫu/ms) * 4 byte * 2 kênh = 392 byte
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX           (((FS / 1000) + 1) * 4 * 2)
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ        (8 * CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX)

#ifdef __cplusplus
}
#endif
