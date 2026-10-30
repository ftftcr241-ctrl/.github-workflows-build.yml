// Các callback điều khiển của TinyUSB Audio (UAC2): clock, volume, mute, alt setting, feedback.
#include <math.h>
#include "tusb.h"
#include "usb_descriptors.h"
#include "app.h"

#define VOL_MIN_DB256  (-60 * 256)     // đơn vị 1/256 dB
#define VOL_MAX_DB256  0
#define VOL_RES_DB256  256             // bước 1 dB

static bool    s_mute[AUDIO_CHANNELS + 1];
static int16_t s_volume[AUDIO_CHANNELS + 1];   // [0] = master

static void update_gain(void) {
    if (s_mute[0]) { g_gain = 0.0f; return; }
    g_gain = powf(10.0f, ((float)s_volume[0] / 256.0f) / 20.0f);
}

// ---------------- Clock source (tần số cố định 48 kHz) ----------------
static bool clock_get_request(uint8_t rhport, audio_control_request_t const *request) {
    if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ) {
        if (request->bRequest == AUDIO_CS_REQ_CUR) {
            audio_control_cur_4_t cur = { .bCur = (int32_t)tu_htole32(AUDIO_SAMPLE_RATE) };
            return tud_audio_buffer_and_schedule_control_xfer(rhport,
                       (tusb_control_request_t const *)request, &cur, sizeof(cur));
        } else if (request->bRequest == AUDIO_CS_REQ_RANGE) {
            audio_control_range_4_n_t(1) rng;
            rng.wNumSubRanges   = tu_htole16(1);
            rng.subrange[0].bMin = (int32_t)tu_htole32(AUDIO_SAMPLE_RATE);
            rng.subrange[0].bMax = (int32_t)tu_htole32(AUDIO_SAMPLE_RATE);
            rng.subrange[0].bRes = 0;
            return tud_audio_buffer_and_schedule_control_xfer(rhport,
                       (tusb_control_request_t const *)request, &rng, sizeof(rng));
        }
    } else if (request->bControlSelector == AUDIO_CS_CTRL_CLK_VALID &&
               request->bRequest == AUDIO_CS_REQ_CUR) {
        audio_control_cur_1_t valid = { .bCur = 1 };
        return tud_audio_buffer_and_schedule_control_xfer(rhport,
                   (tusb_control_request_t const *)request, &valid, sizeof(valid));
    }
    return false;
}

// ---------------- Feature unit (mute + volume master) ----------------
static bool fu_get_request(uint8_t rhport, audio_control_request_t const *request) {
    if (request->bChannelNumber > AUDIO_CHANNELS) return false;

    if (request->bControlSelector == AUDIO_FU_CTRL_MUTE && request->bRequest == AUDIO_CS_REQ_CUR) {
        audio_control_cur_1_t m = { .bCur = s_mute[request->bChannelNumber] };
        return tud_audio_buffer_and_schedule_control_xfer(rhport,
                   (tusb_control_request_t const *)request, &m, sizeof(m));
    }
    if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME) {
        if (request->bRequest == AUDIO_CS_REQ_RANGE) {
            audio_control_range_2_n_t(1) rng;
            rng.wNumSubRanges    = tu_htole16(1);
            rng.subrange[0].bMin = (int16_t)tu_htole16(VOL_MIN_DB256);
            rng.subrange[0].bMax = (int16_t)tu_htole16(VOL_MAX_DB256);
            rng.subrange[0].bRes = tu_htole16(VOL_RES_DB256);
            return tud_audio_buffer_and_schedule_control_xfer(rhport,
                       (tusb_control_request_t const *)request, &rng, sizeof(rng));
        } else if (request->bRequest == AUDIO_CS_REQ_CUR) {
            audio_control_cur_2_t v = { .bCur = (int16_t)tu_htole16(s_volume[request->bChannelNumber]) };
            return tud_audio_buffer_and_schedule_control_xfer(rhport,
                       (tusb_control_request_t const *)request, &v, sizeof(v));
        }
    }
    return false;
}

static bool fu_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf) {
    (void)rhport;
    if (request->bRequest != AUDIO_CS_REQ_CUR) return false;
    if (request->bChannelNumber > AUDIO_CHANNELS) return false;

    if (request->bControlSelector == AUDIO_FU_CTRL_MUTE) {
        if (request->wLength != sizeof(audio_control_cur_1_t)) return false;
        s_mute[request->bChannelNumber] = ((audio_control_cur_1_t const *)buf)->bCur;
        update_gain();
        return true;
    }
    if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME) {
        if (request->wLength != sizeof(audio_control_cur_2_t)) return false;
        s_volume[request->bChannelNumber] = ((audio_control_cur_2_t const *)buf)->bCur;
        update_gain();
        return true;
    }
    return false;
}

// ---------------- Callback TinyUSB ----------------
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    audio_control_request_t const *request = (audio_control_request_t const *)p_request;
    if (request->bEntityID == UAC2_ENTITY_CLOCK)        return clock_get_request(rhport, request);
    if (request->bEntityID == UAC2_ENTITY_FEATURE_UNIT) return fu_get_request(rhport, request);
    return false;
}

bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *buf) {
    audio_control_request_t const *request = (audio_control_request_t const *)p_request;
    if (request->bEntityID == UAC2_ENTITY_FEATURE_UNIT) return fu_set_request(rhport, request, buf);
    if (request->bEntityID == UAC2_ENTITY_CLOCK)        return true;   // tần số cố định, chấp nhận và bỏ qua
    return false;
}

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport; (void)p_request;
    g_flush = true;                    // alt setting đổi: xả đệm, đệm lại từ đầu
    return true;
}

bool tud_audio_set_itf_close_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport; (void)p_request;
    g_flush = true;
    return true;
}

// Feedback do ứng dụng tự tính (fb_task() trong main.c) nên tắt phương pháp tự động của thư viện.
void tud_audio_feedback_params_cb(uint8_t func_id, uint8_t alt_itf, audio_feedback_params_t *feedback_param) {
    (void)func_id; (void)alt_itf;
    feedback_param->method = AUDIO_FEEDBACK_METHOD_DISABLED;
}
