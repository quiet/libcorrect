#ifndef CORRECT_CONVOLUTIONAL_LOOKUP
#define CORRECT_CONVOLUTIONAL_LOOKUP
#include "correct/convolutional.h"

#ifdef _MSC_VER
#  include <intrin.h>
#  define __builtin_popcount __popcnt
#endif

typedef unsigned int distance_pair_key_t;
typedef uint32_t output_pair_t;
typedef uint32_t distance_pair_t;

typedef struct {
    distance_pair_key_t *keys;
    output_pair_t *outputs;
    output_pair_t output_mask;
    unsigned int output_width;
    size_t outputs_len;
    distance_pair_t *distances;
} pair_lookup_t;

DLL_EXPORT void fill_table(unsigned int order,
                unsigned int rate,
                const polynomial_t *poly,
                unsigned int *table);
DLL_EXPORT pair_lookup_t pair_lookup_create(unsigned int rate,
                                 unsigned int order,
                                 const unsigned int *table);
DLL_EXPORT void pair_lookup_destroy(pair_lookup_t pairs);
DLL_EXPORT void pair_lookup_fill_distance(pair_lookup_t pairs, distance_t *distances);
#endif
