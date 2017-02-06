#include "correct/convolutional.h"

// measure the hamming distance of two bit strings
// implemented as population count of x XOR y
inline distance_t metric_distance(unsigned int x, unsigned int y) {
    return __builtin_popcount(x ^ y);
}

inline distance_t metric_soft_distance_linear(unsigned int hard_x, const uint8_t *soft_y, size_t len) {
    distance_t dist = 0;
    for (unsigned int i = 0; i < len; i++) {
        unsigned int soft_x = ((int8_t)(0) - (hard_x & 1)) & 0xff;
        hard_x >>= 1;
        int d = soft_y[i] - soft_x;
        dist += (d < 0) ? -d : d;
    }
    return dist;
}

DLL_EXPORT distance_t metric_soft_distance_quadratic(unsigned int hard_x, const uint8_t *soft_y, size_t len);
