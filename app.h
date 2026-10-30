#pragma once
#include <stdbool.h>

#define FS 48000                        // tần số lấy mẫu cố định

extern volatile float g_gain;           // âm lượng tuyến tính (0..1), cập nhật từ USB
extern volatile bool  g_flush;          // core 0 đặt true khi stream USB đổi trạng thái
