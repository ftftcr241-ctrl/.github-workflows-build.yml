#include <string.h>
#include "tusb.h"
#include "pico/unique_id.h"
#include "usb_descriptors.h"

// ---------------- Device descriptor ----------------
static tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCafe,      // VID thử nghiệm của TinyUSB, KHÔNG dùng khi bán sản phẩm
    .idProduct          = 0x4010,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

// ---------------- Configuration descriptor (UAC2 loa stereo + feedback) ----------------
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + XOVER_AUDIO_DESC_LEN)

#define XOVER_AUDIO_DESCRIPTOR(_itfnum, _stridx, _epout, _epoutsize, _epfb) \
    /* IAD */ \
    TUD_AUDIO_DESC_IAD(/*_firstitf*/ _itfnum, /*_nitfs*/ 0x02, /*_stridx*/ 0x00), \
    /* Audio Control interface */ \
    TUD_AUDIO_DESC_STD_AC(/*_itfnum*/ _itfnum, /*_nEPs*/ 0x00, /*_stridx*/ _stridx), \
    TUD_AUDIO_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO_FUNC_DESKTOP_SPEAKER, \
        /*_totallen*/ TUD_AUDIO_DESC_CLK_SRC_LEN + TUD_AUDIO_DESC_INPUT_TERM_LEN \
                      + TUD_AUDIO_DESC_OUTPUT_TERM_LEN + TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL_LEN, \
        /*_ctrl*/ AUDIO_CS_AS_INTERFACE_CTRL_LATENCY_POS), \
    /* Clock source: nội bộ, host có thể đặt tần số (chỉ 1 giá trị) */ \
    TUD_AUDIO_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_CLOCK, /*_attr*/ 3, /*_ctrl*/ 7, \
        /*_assocTerm*/ UAC2_ENTITY_INPUT_TERMINAL, /*_stridx*/ 0x00), \
    /* Input terminal: USB streaming, 2 kênh */ \
    TUD_AUDIO_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_INPUT_TERMINAL, \
        /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, \
        /*_clkid*/ UAC2_ENTITY_CLOCK, /*_nchannelslogical*/ 0x02, \
        /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, \
        /*_ctrl*/ 0 * (AUDIO_CTRL_R << AUDIO_IN_TERM_CTRL_POS_COPY_PROT), /*_stridx*/ 0x00), \
    /* Feature unit: chỉ master mute + volume */ \
    TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL(/*_unitid*/ UAC2_ENTITY_FEATURE_UNIT, \
        /*_srcid*/ UAC2_ENTITY_INPUT_TERMINAL, \
        /*_ctrlch0master*/ (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS) \
                         | (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS), \
        /*_ctrlch1*/ 0, /*_ctrlch2*/ 0, /*_stridx*/ 0x00), \
    /* Output terminal: loa */ \
    TUD_AUDIO_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_OUTPUT_TERMINAL, \
        /*_termtype*/ AUDIO_TERM_TYPE_OUT_DESKTOP_SPEAKER, /*_assocTerm*/ 0x00, \
        /*_srcid*/ UAC2_ENTITY_FEATURE_UNIT, /*_clkid*/ UAC2_ENTITY_CLOCK, \
        /*_ctrl*/ 0x0000, /*_stridx*/ 0x00), \
    /* Audio Streaming interface: alt 0 (không băng thông) */ \
    TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)((_itfnum) + 1), /*_altset*/ 0x00, \
        /*_nEPs*/ 0x00, /*_stridx*/ 0x00), \
    /* Audio Streaming interface: alt 1 (phát nhạc, 2 endpoint: data + feedback) */ \
    TUD_AUDIO_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)((_itfnum) + 1), /*_altset*/ 0x01, \
        /*_nEPs*/ 0x02, /*_stridx*/ 0x00), \
    TUD_AUDIO_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_INPUT_TERMINAL, /*_ctrl*/ AUDIO_CTRL_NONE, \
        /*_formattype*/ AUDIO_FORMAT_TYPE_I, /*_formats*/ AUDIO_DATA_FORMAT_TYPE_I_PCM, \
        /*_nchannelsphysical*/ 0x02, /*_channelcfg*/ AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, \
        /*_stridx*/ 0x00), \
    TUD_AUDIO_DESC_TYPE_I_FORMAT(AUDIO_BYTES_PER_SAMPLE, AUDIO_BITS_USED), \
    /* Endpoint dữ liệu: isochronous, asynchronous */ \
    TUD_AUDIO_DESC_STD_AS_ISO_EP(/*_ep*/ _epout, \
        /*_attr*/ (uint8_t)((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ASYNCHRONOUS \
                            | (uint8_t)TUSB_ISO_EP_ATT_DATA), \
        /*_maxEPsize*/ _epoutsize, /*_interval*/ 0x01), \
    TUD_AUDIO_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, \
        /*_ctrl*/ AUDIO_CTRL_NONE, /*_lockdelayunit*/ AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, \
        /*_lockdelay*/ 0x0000), \
    /* Endpoint feedback */ \
    TUD_AUDIO_DESC_STD_AS_ISO_FB_EP(/*_ep*/ _epfb, /*_interval*/ 0x01)

static uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    XOVER_AUDIO_DESCRIPTOR(ITF_NUM_AUDIO_CONTROL, 4, EPNUM_AUDIO_OUT, AUDIO_EP_OUT_SIZE, EPNUM_AUDIO_FB)
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

// ---------------- String descriptors ----------------
static char const *string_desc[] = {
    NULL,                         // 0: ngôn ngữ (xử lý riêng)
    "DIY Audio",                  // 1: nhà sản xuất
    "Pico2 Active Crossover",     // 2: tên sản phẩm
    NULL,                         // 3: số serial (lấy từ ID của board)
    "Active Crossover 3-way"      // 4: tên interface audio
};

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t desc[32];
    char serial[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];
    uint8_t chr_count;

    if (index == 0) {
        desc[1] = 0x0409;         // English (US)
        chr_count = 1;
    } else {
        char const *str;
        if (index == 3) {
            pico_get_unique_board_id_string(serial, sizeof serial);
            str = serial;
        } else if (index < sizeof(string_desc) / sizeof(string_desc[0]) && string_desc[index]) {
            str = string_desc[index];
        } else {
            return NULL;
        }
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++) desc[1 + i] = str[i];
    }
    desc[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return desc;
}
