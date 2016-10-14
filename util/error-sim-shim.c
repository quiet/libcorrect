#include "correct/util/error-sim-shim.h"

void conv_fec27_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    update_viterbi27_blk(conv_v, soft, soft_len / 2);
    size_t n_decoded_bits = (soft_len - 8) / 2;
    chainback_viterbi27(conv_v, msg, n_decoded_bits, 0);
}

void conv_fec29_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    update_viterbi29_blk(conv_v, soft, soft_len / 2);
    size_t n_decoded_bits = (soft_len - 10) / 2;
    chainback_viterbi29(conv_v, msg, n_decoded_bits, 0);
}

void conv_fec39_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    update_viterbi39_blk(conv_v, soft, soft_len / 3);
    size_t n_decoded_bits = (soft_len - 10) / 3;
    chainback_viterbi39(conv_v, msg, n_decoded_bits, 0);
}

void conv_fec615_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg) {
    update_viterbi615_blk(conv_v, soft, soft_len / 6);
    size_t n_decoded_bits = (soft_len - 16) / 6;
    chainback_viterbi615(conv_v, msg, n_decoded_bits, 0);
}
