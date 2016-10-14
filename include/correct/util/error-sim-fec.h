#include "correct/util/error-sim.h"

#include <fec.h>

void conv_fec_decode(void *conv_v, uint8_t *soft, size_t soft_len, uint8_t *msg);
