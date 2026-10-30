#pragma once
#include "app.h"

// ID các entity trong topology UAC2
#define UAC2_ENTITY_INPUT_TERMINAL   0x01
#define UAC2_ENTITY_FEATURE_UNIT     0x02
#define UAC2_ENTITY_OUTPUT_TERMINAL  0x03
#define UAC2_ENTITY_CLOCK            0x04

enum {
    ITF_NUM_AUDIO_CONTROL = 0,
    ITF_NUM_AUDIO_STREAMING,
    ITF_NUM_TOTAL
};

#define EPNUM_AUDIO_OUT   0x01   // dữ liệu âm thanh từ host
#define EPNUM_AUDIO_FB    0x81   // feedback về host

#define AUDIO_SAMPLE_RATE      FS
#define AUDIO_BYTES_PER_SAMPLE 4      // slot 32-bit
#define AUDIO_BITS_USED        24     // 24-bit có nghĩa, căn trái
#define AUDIO_CHANNELS         2
#define AUDIO_EP_OUT_SIZE      (((AUDIO_SAMPLE_RATE / 1000) + 1) * AUDIO_BYTES_PER_SAMPLE * AUDIO_CHANNELS)
