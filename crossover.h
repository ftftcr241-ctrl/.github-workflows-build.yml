#pragma once
#include <math.h>

// ---- Biquad float, dang Direct Form II Transposed ----
typedef struct { float b0, b1, b2, a1, a2, z1, z2; } bq_t;

static inline float bq_run(bq_t *f, float x) {
    float y = f->b0 * x + f->z1;
    f->z1 = f->b1 * x - f->a1 * y + f->z2;
    f->z2 = f->b2 * x - f->a2 * y;
    return y;
}

typedef enum { BQ_LP, BQ_HP, BQ_AP } bq_type_t;

// He so RBJ cookbook. Khong reset z1/z2 de doi tan so khong bi click.
static inline void bq_set(bq_t *f, bq_type_t t, float fc, float q, float fs) {
    float w = 2.0f * (float)M_PI * fc / fs;
    float c = cosf(w), s = sinf(w), al = s / (2.0f * q);
    float b0, b1, b2, a0 = 1.0f + al, a1 = -2.0f * c, a2 = 1.0f - al;
    switch (t) {
        case BQ_LP: b0 = (1.0f - c) * 0.5f; b1 = 1.0f - c;     b2 = b0;        break;
        case BQ_HP: b0 = (1.0f + c) * 0.5f; b1 = -(1.0f + c);  b2 = b0;        break;
        default:    b0 = 1.0f - al;         b1 = -2.0f * c;    b2 = 1.0f + al; break;
    }
    f->b0 = b0 / a0; f->b1 = b1 / a0; f->b2 = b2 / a0;
    f->a1 = a1 / a0; f->a2 = a2 / a0;
}

// ---- Crossover 3 dai, Linkwitz-Riley bac 4 (2 biquad Butterworth noi tiep) ----
//   in -> LP1 (f1) -> allpass(f2) ------------------> LOW
//   in -> HP1 (f1) -> LP2 (f2) ---------------------> MID
//              \----> HP2 (f2) ---------------------> HIGH
// Allpass bac 2 tai f2 cho dai LOW bu pha vi LOW khong di qua bo loc f2,
// nhờ vay LOW + MID + HIGH cho tong bien do phang.
typedef struct {
    bq_t lp1[2], hp1[2], lp2[2], hp2[2], ap2;
    float gain[3];
} xover_t;

#define BUTTER_Q 0.70710678f

static inline void xover_init(xover_t *x, float f1, float f2, float fs) {
    for (int i = 0; i < 2; i++) {
        bq_set(&x->lp1[i], BQ_LP, f1, BUTTER_Q, fs);
        bq_set(&x->hp1[i], BQ_HP, f1, BUTTER_Q, fs);
        bq_set(&x->lp2[i], BQ_LP, f2, BUTTER_Q, fs);
        bq_set(&x->hp2[i], BQ_HP, f2, BUTTER_Q, fs);
    }
    bq_set(&x->ap2, BQ_AP, f2, BUTTER_Q, fs);
    x->gain[0] = x->gain[1] = x->gain[2] = 1.0f;
}

static inline void xover_run(xover_t *x, float in, float out[3]) {
    float lo  = bq_run(&x->ap2, bq_run(&x->lp1[1], bq_run(&x->lp1[0], in)));
    float h   = bq_run(&x->hp1[1], bq_run(&x->hp1[0], in));
    float mid = bq_run(&x->lp2[1], bq_run(&x->lp2[0], h));
    float hi  = bq_run(&x->hp2[1], bq_run(&x->hp2[0], h));
    out[0] = lo  * x->gain[0];
    out[1] = mid * x->gain[1];
    out[2] = hi  * x->gain[2];
}
