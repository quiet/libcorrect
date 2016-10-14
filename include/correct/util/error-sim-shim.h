#include "correct/util/error-sim.h"
#include "fec_shim.h"

void conv_shim27_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg);
void conv_shim29_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg);
void conv_shim39_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg);
void conv_shim615_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg);
