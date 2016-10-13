#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>

#include "correct.h"
#include "correct-sse.h"

size_t distance(uint8_t *a, uint8_t *b, size_t len);
void gaussian(double *res, size_t n_res, double sigma);

void encode_bpsk(uint8_t *msg, double *voltages, size_t n_syms, double bpsk_voltage);
void byte2bit(uint8_t *bytes, uint8_t *bits, size_t n_bits);
void decode_bpsk(uint8_t *soft, uint8_t *msg, size_t n_syms);
void decode_bpsk_soft(double *voltages, uint8_t *soft, size_t n_syms, double bpsk_voltage);
double log2amp(double l);
double amp2log(double a);
double sigma_for_eb_n0(double eb_n0, double bpsk_bit_energy);
void build_white_noise(double *noise, size_t n_syms, double eb_n0, double bpsk_bit_energy);
void add_white_noise(double *signal, double *noise, size_t n_syms);

typedef struct {
    uint8_t *msg_out;
    size_t msg_len;
    uint8_t *encoded;
    double *v;
    double *corrupted;
    uint8_t *soft;
    double *noise;
    size_t enclen;
    size_t enclen_bytes;
    void (*encode)(void *, uint8_t *msg, size_t msg_len, uint8_t *encoded);
    void *encoder;
    void (*decode)(void *, uint8_t *soft, size_t soft_len, uint8_t *msg);
    void *decoder;
} conv_testbench;

conv_testbench *resize_conv_testbench(conv_testbench *scratch, correct_convolutional *conv, size_t msg_len);
void free_scratch(conv_testbench *scratch);
int test_conv_noise(correct_convolutional *conv, uint8_t *msg, size_t n_bytes,
                    conv_testbench *scratch, double bpsk_voltage);
conv_testbench *resize_sse_conv_testbench(conv_testbench *scratch, correct_convolutional_sse *conv, size_t msg_len);
int test_sse_conv_noise(correct_convolutional_sse *conv, uint8_t *msg, size_t n_bytes,
                    conv_testbench *scratch, double bpsk_voltage);
