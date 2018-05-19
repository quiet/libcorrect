#ifndef CORRECT_REED_SOLOMON_FIELD
#define CORRECT_REED_SOLOMON_FIELD
#include "correct/reed-solomon.h"

/*
field_t field_create(field_operation_t primitive_poly);
void field_destroy(field_t field);
field_element_t field_add(field_t field, field_element_t l, field_element_t r);
field_element_t field_sub(field_t field, field_element_t l, field_element_t r);
field_element_t field_sum(field_t field, field_element_t elem, unsigned int n);
field_element_t field_mul(field_t field, field_element_t l, field_element_t r);
field_element_t field_div(field_t field, field_element_t l, field_element_t r);
field_logarithm_t field_mul_log(field_t field, field_logarithm_t l, field_logarithm_t r);
field_logarithm_t field_div_log(field_t field, field_logarithm_t l, field_logarithm_t r);
field_element_t field_mul_log_element(field_t field, field_logarithm_t l, field_logarithm_t r);
field_element_t field_pow(field_t field, field_element_t elem, int pow);
*/

static inline field_element_t field_mul_log_element(field_t field, field_logarithm_t l, field_logarithm_t r) {
    // like field_mul_log, but returns a field_element_t
    // because we are doing lookup here, we can safely skip the wrapover check
    field_operation_t res = (field_operation_t)l + (field_operation_t)r;
    return field.exp[res];
}

static inline field_t field_create(field_operation_t primitive_poly) {
    unsigned int width = 0;
    field_operation_t temp_poly = primitive_poly >> 1;
    while (temp_poly) {
        temp_poly >>= 1;
        ++width;
    }
    const unsigned int field_size = (1 << width);
    const unsigned int largest_element = field_size - 1;
    // in GF(2^width)
    // log and exp
    // bits are in GF(2), compute alpha^val in GF(2^width)
    // exp should be of size 2 * 2^width so that it can hold a "wraparound" which prevents some modulo ops
    // log should be of size 2^width. no wraparound here, the indices into this table are field elements
    field_element_t *exp = malloc(2 * field_size * sizeof(field_element_t));
    field_logarithm_t *log = malloc(field_size * sizeof(field_logarithm_t));

    // assume alpha is a primitive element, p(x) (primitive_poly) irreducible in GF(2^width)
    // addition is xor
    // subtraction is addition (also xor)
    // e.g. x^5 + x^4 + x^4 + x^2 + 1 = x^5 + x^2 + 1
    // each row of exp contains the field element found by exponentiating
    //   alpha by the row index
    // each row of log contains the coefficients of
    //   alpha^7 + alpha^6 + alpha^5 + alpha^4 + alpha^3 + alpha^2 + alpha + 1
    // as 8 bits packed into one byte

    field_operation_t element = 1;
    exp[0] = (field_element_t)element;
    log[0] = (field_logarithm_t)0;  // really, it's undefined. we shouldn't ever access this
    for (field_operation_t i = 1; i < 2 * field_size; i++) {
        element = element * 2;
        element = (element > largest_element) ? (element ^ primitive_poly) : element;
        exp[i] = (field_element_t)element;
        if (i <= largest_element) {
            log[element] = (field_logarithm_t)i;
        }
    }

    field_t field;
    *(field_element_t **)&field.exp = exp;
    *(field_logarithm_t **)&field.log = log;
    *(unsigned int *)&field.field_size = field_size;
    *(unsigned int *)&field.largest_element = largest_element;

    return field;
}

static inline void field_destroy(field_t field) {
    free(*(field_element_t **)&field.exp);
    free(*(field_element_t **)&field.log);
}

static inline field_element_t field_add(field_t field, field_element_t l, field_element_t r) {
    return l ^ r;
}

static inline field_element_t field_sub(field_t field, field_element_t l, field_element_t r) {
    return l ^ r;
}

static inline field_element_t field_sum(field_t field, field_element_t elem, unsigned int n) {
    // we'll do a closed-form expression of the sum, although we could also
    //   choose to call field_add n times

    // since the sum is actually the bytewise XOR operator, this suggests two
    // kinds of values: n odd, and n even

    // if you sum once, you have coeff
    // if you sum twice, you have coeff XOR coeff = 0
    // if you sum thrice, you are back at coeff
    // an even number of XORs puts you at 0
    // an odd number of XORs puts you back at your value

    // so, just throw away all the even n
    return (n % 2) ? elem : 0;
}

static inline field_element_t field_mul(field_t field, field_element_t l, field_element_t r) {
    if (l == 0 || r == 0) {
        return 0;
    }
    // multiply two field elements by adding their logarithms.
    // yep, get your slide rules out
    field_operation_t res = (field_operation_t)field.log[l] + (field_operation_t)field.log[r];

    // if coeff exceeds largest_element, we would normally have to wrap it back around
    // alpha^largest_element = 1; alpha^(largest_element + 1) = alpha^largest_element * alpha^1 = alpha^1
    // however, we've constructed exponentiation table so that
    //   we can just directly lookup this result
    // the result must be clamped to [0, (2 * largest_element) + 1]
    // the greatest we can see at this step is alpha^largest_element * alpha^largest_element
    //   = alpha^(2 * largest_element)
    return field.exp[res];
}

static inline field_element_t field_div(field_t field, field_element_t l, field_element_t r) {
    if (l == 0) {
        return 0;
    }

    if (r == 0) {
        // XXX ???
        return 0;
    }

    // division as subtraction of logarithms

    // if rcoeff is larger, then log[l] - log[r] wraps under
    // so, instead, always add largest_element. in some cases, we'll wrap over, but
    // that's ok because the exp table runs up to (2 * largest_element) + 1.
    field_operation_t res = (field_operation_t)field.largest_element + (field_operation_t)field.log[l] - (field_operation_t)field.log[r];
    return field.exp[res];
}

static inline field_logarithm_t field_mul_log(field_t field, field_logarithm_t l, field_logarithm_t r) {
    // this function performs the equivalent of field_mul on two logarithms
    // we save a little time by skipping the lookup step at the beginning
    field_operation_t res = (field_operation_t)l + (field_operation_t)r;

    // because we arent using the table, the value we return must be a valid logarithm
    // which we have decided must live in [0, largest_element]
    // ensuring this makes it so that multiple muls will not reach past the end of the
    // exp table whenever we finally convert back to an element
    if (res > field.largest_element) {
        return (field_logarithm_t)(res - field.largest_element);
    }
    return (field_logarithm_t)res;
}

static inline field_logarithm_t field_div_log(field_t field, field_logarithm_t l, field_logarithm_t r) {
    // like field_mul_log, this performs field_div without going through a field_element_t
    field_operation_t res = (field_operation_t)field.largest_element + (field_operation_t)l - (field_operation_t)r;
    if (res > field.largest_element) {
        return (field_logarithm_t)(res - field.largest_element);
    }
    return (field_logarithm_t)res;
}

static inline field_element_t field_pow(field_t field, field_element_t elem, int pow) {
    // take the logarithm, multiply, and then "exponentiate"
    // n.b. the exp table only considers powers of alpha, the primitive element
    // but here we have an arbitrary coeff
    field_logarithm_t log = field.log[elem];
    int res_log = log * pow;
    int mod = res_log % field.largest_element;
    if (mod < 0) {
        mod += field.largest_element;
    }
    return field.exp[mod];
}
#endif
