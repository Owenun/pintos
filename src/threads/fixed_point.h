#include <stdint.h>

typedef int32_t fixedpt;

#define fixedpt_p (17)
#define fixedpt_q (14)

static int fixedpt_f = 1 << fixedpt_q ; 

static inline fixedpt fixedpt_fromint(int a) {
    return a * fixedpt_f;
}

static inline int fixedpt_toint(fixedpt A) {
    return A / fixedpt_f;
}

static inline int fixedpt_toint_nearest(fixedpt A) {
    return (A >= 0) ? ((A + fixedpt_f / 2) / fixedpt_f) : ((A - fixedpt_f / 2) / fixedpt_f );
}

static inline fixedpt fixedpt_add(fixedpt A, fixedpt B) {
    return A + B;
}

static inline fixedpt fixedpt_sub(fixedpt A, fixedpt B) {
    return A - B;
}

static inline fixedpt fixedpt_addi(fixedpt A, int b) {
    return A + b * fixedpt_f;
} 

static inline fixedpt fixedpt_subi(fixedpt A, int b) {
    return A - b * fixedpt_f;
}

static inline fixedpt fixedpt_div(fixedpt A, fixedpt B) {
    return (fixedpt) (((int64_t) A ) * fixedpt_f / B);
}

static inline fixedpt fixedpt_mul(fixedpt A, fixedpt B) {
    return (fixedpt) (((int64_t) A ) * B / fixedpt_f);
}

static inline fixedpt fixedpt_muli(fixedpt A, int b) {
    return A * b;
}

static inline fixedpt fixedpt_divi(fixedpt A, int b) {
    return A / b;

}


