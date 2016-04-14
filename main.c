#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>

#include <fec.h>

// https://www.youtube.com/watch?v=b3_lVSrPB6w

// shift in most significant bit every time, one byte at a time
// shift register takes most recent bit on right, shifts left
// poly is written in same order, just & mask message w/ poly

typedef struct {
    const uint8_t *table;  // size 2**order
    size_t rate;  // e.g. 2, 3...
    size_t order; // e.g. 7, 9...
    size_t numstates;  // 2**order
    unsigned int *errors[2];
    uint64_t *winners;
} correct_conv;

typedef uint16_t shift_register_t;
typedef uint16_t polynomial_t;
typedef uint64_t path_t;

//polynomial_t r12k7[] = {0133, 0171};
//polynomial_t r12k7[] = {0171, 0133};
//polynomial_t r12k7[] = {0117, 0155};
polynomial_t r12k7[] = {0155, 0117};


// population count
static unsigned int popcnt(unsigned int x) {
    unsigned int count = 0;
    for (size_t i = 0; i < sizeof(unsigned int) * 8; i++) {
        count += x & 1;
        x >>= 1;
    }
    return count;
}

// table has numstates rows
// each row contains all of the polynomial output bits concatenated together
// e.g. for rate 2, we have 2 bits in each row
// the first poly gets the LEAST significant bit, last poly gets most significant
static void fill_table(correct_conv *conv, polynomial_t *poly, uint8_t *table) {
    for (shift_register_t i = 0; i < conv->numstates; i++) {
        uint8_t out = 0;
        unsigned int mask = 1;
        for (size_t j = 0; j < conv->rate; j++) {
            out |= (__builtin_popcount(i & poly[j]) % 2) ? mask : 0;
            mask <<= 1;
        }
        table[i] = out;
    }
}

correct_conv *correct_conv_create(size_t rate, size_t order, polynomial_t *poly) {
    if (order > 8 * sizeof(shift_register_t)) {
        printf("order must be smaller than 8 * sizeof(shift_register_t)\n");
        return NULL;
    }
    if (rate < 2) {
        printf("rate must be 2 or greater\n");
        return NULL;
    }
    correct_conv *conv = malloc(sizeof(correct_conv));
    conv->order = order;
    conv->rate = rate;
    conv->numstates = 1 << order;
    conv->errors[0] = NULL;
    conv->errors[1] = NULL;
    conv->winners = NULL;

    uint8_t *table = malloc(sizeof(uint8_t) * (1 << order));
    fill_table(conv, poly, table);
    *(uint8_t **)&conv->table = table;

    return conv;
}

void correct_conv_destroy(correct_conv *conv) {
    free(*(uint8_t **)&conv->table);
    free(conv);
    if (conv->errors[0]) {
        free(conv->errors[0]);
    }
    if (conv->errors[1]) {
        free(conv->errors[1]);
    }
    if (conv->winners) {
        free(conv->winners);
    }
}

// rate * (msg_len + order)
size_t correct_conv_encode_len(correct_conv *conv, size_t msg_len) {
    size_t msgbits = 8 * msg_len;
    size_t encodedbits = conv->rate * (msgbits + conv->order);
    return (encodedbits % 8) ? (encodedbits / 8 + 1) : (encodedbits / 8);
}

// assume that encoded length is long enough?
size_t correct_conv_encode(correct_conv *conv, const uint8_t *msg, size_t msg_len, uint8_t *encoded) {
    // convolutional code convolves filter coefficients, given by the polynomial, with some history from
    // our message. the history is stored as single subsequent bits in shiftregister
    shift_register_t shiftregister = 0;

    // shiftmask is the shiftregister bit mask that removes bits that extend beyond order
    // e.g. if order is 7, then remove the 8th bit and beyond
    unsigned int shiftmask = 0;
    for (size_t i = 0; i < conv->order; i++) {
        shiftmask <<= 1;
        shiftmask |= 1;
    }

    // we build up a full byte and then write to encoded
    uint8_t encoded_byte = 0;
    // how many bits are in this byte so far?
    size_t encoded_byte_len = 0;
    // index into encoded
    size_t encoded_index = 0;

    for (size_t i = 0; i < msg_len; i++) {
        // shift most significant bit from byte and move down one bit at a time
        // once we have the least significant bit, move to the next byte
        for (unsigned int mask = 0x80; mask > 0; mask >>= 1) {
            // shiftregister has oldest bits on left, newest on right
            shiftregister <<= 1;
            shiftregister |= (msg[i] & mask) ? 1 : 0;
            shiftregister &= shiftmask;

            // we do direct lookup of our convolutional output here
            // all of the bits from this convolution are stored in this row
            uint8_t out = conv->table[shiftregister];
            for (size_t j = 0; j < conv->rate; j++) {
                // take the least significant bit off of the conv. output and put it
                // on our so-far encoded byte
                encoded_byte |= out & 1;
                out >>= 1;
                encoded_byte_len++;

                if (encoded_byte_len == 8) {
                    // 8 bits in a byte -- move to the next byte
                    encoded[encoded_index] = encoded_byte;
                    encoded_index++;
                    encoded_byte_len = 0;
                    encoded_byte = 0;
                } else {
                    encoded_byte <<= 1;
                }
            }
        }
    }

    // now flush the shiftregister
    // this is simply running the loop as above but without any new inputs
    // or rather, the new input string is all 0s
    // TODO DRY up the encoded byte writing logic here
    for (size_t i = 0; i < conv->order; i++) {
        shiftregister <<= 1;
        shiftregister &= shiftmask;
        uint8_t out = conv->table[shiftregister];
        for (size_t j = 0; j < conv->rate; j++) {
            encoded_byte |= out & 1;
            out >>= 1;
            encoded_byte_len++;
            if (encoded_byte_len == 8) {
                encoded[encoded_index] = encoded_byte;
                encoded_index++;
                encoded_byte_len = 0;
                encoded_byte = 0;
            } else {
                encoded_byte <<= 1;
            }
        }
    }

    // 0-fill any remaining bits on our final byte
    if (encoded_byte_len != 0) {
        encoded_byte <<= (8 - encoded_byte_len);
        encoded[encoded_index] = encoded_byte;
        encoded_index++;
        encoded_byte_len = 0;
    }

    return encoded_index;
}

// measure the hamming distance of two bit strings
// implemented as population count of x XOR y
static uint8_t distance(uint8_t x, uint8_t y) {
    return __builtin_popcount((uint8_t)(x ^ y));
}

// perform viterbi decoding
// hard decoder
size_t correct_conv_decode(correct_conv *conv, const uint8_t *encoded, size_t encoded_len, uint8_t *msg) {
    if (encoded_len % conv->rate) {
        printf("encoded length of message must be a multiple of rate\n");
        return 0;
    }

    size_t sets = (8 * encoded_len) / conv->rate;

    uint8_t encoded_byte = encoded[0];
    size_t encoded_index = 0;
    size_t encoded_mask = 0x80;

    uint8_t decoded_byte = 0;
    size_t decoded_index = 0;
    size_t decoded_length = 0;

    // save two error metrics, one for last round and one for this
    // (double buffer)
    // the error metric is the aggregated number of bit errors found
    //   at a given path which terminates at a particular shift register state
    if (!conv->errors[0]) {
        conv->errors[0] = calloc(sizeof(unsigned int), conv->numstates);
    }
    if (!conv->errors[1]) {
        conv->errors[1] = calloc(sizeof(unsigned int), conv->numstates);
    }
    // which buffer are we using, 0 or 1?
    size_t errors_index = 0;

    // we store a finite number of tracebacks, or "winners" at each shift register state
    // this information is stored per set of outputs (time slice)
    // we store a single bit per time slice, speficially, the bit that was shifted OUT OF the winning register
    // for each time slice, for any given shift register state, two different predecessor states will lead to this state
    // those predecessor states are the same except for their high order bit (the oldest bit in the state)
    // and we store in this table that high order bit for whichever of the two had the lowest aggregated error metric

    // we limit history to go back as far as 5 * the order of our polynomial
    size_t winner_cap = 5 * conv->order;
    // must fit in a uint64_t
    assert(64 >= winner_cap);
    // mask out any bits that extend beyond the history we want to keep
    uint64_t winner_keep_mask = ((uint64_t)1 << winner_cap) - 1;
    // winners is a compact history representation for every shift register state, one bit per time slice
    if (!conv->winners) {
        conv->winners = calloc(sizeof(uint64_t), conv->numstates);
    }
    // how many valid entries are there in each row in winners?
    // at steady state, this == winner_cap, but will be less during warmup
    size_t winner_len = 0;
    // a mask to get the high order bit from the shift register
    const shift_register_t highbit = 1 << (conv->order - 1);

    // this is our loop counter. we have two very different loops so we simply persist this between them
    size_t i = 0;

    // there are two warmup phases
    // no outputs are generated during warmup

    // first phase: load shiftregister up from 0 (order goes from 1 to conv->order)
    // we are building up error metrics for the first order bits
    for (; i < conv->order - 1 && i < sets; i++) {
        uint8_t out = 0;
        unsigned int outmask = 1;
        // peel off rate bits from encoded to recover the same `out` as in the encoding process
        // the difference being that this `out` will have the channel noise/errors applied
        for (size_t j = 0; j < conv->rate; j++) {
            out |= (encoded_byte & encoded_mask) ? outmask : 0;
            // we place most significant bits onto the least significant bits of out
            // this matches the process produced in the encoder and the order found in the table
            outmask <<= 1;
            encoded_mask >>= 1;
            if (!encoded_mask) {
                encoded_mask = 0x80;
                encoded_index++;
                encoded_byte = encoded[encoded_index];
            }
        }
        const unsigned int *lasterrors = conv->errors[errors_index];
        errors_index = (errors_index + 1) % 2;
        unsigned int *thiserrors = conv->errors[errors_index];
        // walk all of the state we have so far
        for (size_t j = 0; j < (1 << (i + 1)); j+= 1) {
            unsigned int last = j >> 1;
            thiserrors[j] = distance(conv->table[j], out) + lasterrors[last];
        }
    }

    // normally, if we want to get historical bits, we have to walk the history table
    // since this is kind of slow, we'll track the best path so that we only
    // need to walk the table again if the best path changes
    path_t best_path_history = 0;
    path_t best_path_mask = (1 << (conv->order - 1)) - 1;

    // now the shift register is at steady state
    // we lift the best path state out of the loop so it can be used in flushing (after this loop)
    shift_register_t bestpath;
    for (; i < sets; i++) {
        // do some bit arithmetic to find our next set of polynomial outputs
        uint8_t out = 0;
        unsigned int outmask = 1;
        for (size_t j = 0; j < conv->rate; j++) {
            out |= (encoded_byte & encoded_mask) ? outmask : 0;
            outmask <<= 1;
            encoded_mask >>= 1;
            if (!encoded_mask) {
                encoded_mask = 0x80;
                encoded_index++;
                encoded_byte = encoded[encoded_index];
            }
        }
        // lasterrors are the aggregate bit errors for the states of shiftregister for the previous time slice
        const unsigned int *lasterrors = conv->errors[errors_index];
        errors_index = (errors_index + 1) % 2;
        // aggregate bit errors for this time slice
        unsigned int *thiserrors = conv->errors[errors_index];

        // walk through all states, ignoring oldest bit
        // we will track a best register state (path) and the number of bit errors at that path at this time slice
        unsigned int leasterror = UINT_MAX;
        // this loop considers two paths per iteration (high order bit set, clear)
        // so, it only runs numstates/2 iterations
        // we'll update the history for every state and find the path with the least aggregated bit errors
        for (shift_register_t j = 0; j < conv->numstates/2; j += 1) {
            // j contains the predecessor state for the state we'll be going to
            // that is, the state we're going *to* is arrived at from the predecessor by shifting in
            //    a 0 or 1 (we handle both cases in this iteration)

            // candidate predecessor A -- high bit cleared
            shift_register_t zero_pred = j;
            unsigned int zero_errors = lasterrors[zero_pred];

            // candidate predecessor B -- high bit set
            shift_register_t one_pred = j | highbit;
            unsigned int one_errors = lasterrors[one_pred];

            // decide which predecessor wins
            // take the lesser of these two distances and drop other transition (dynamic programming)
            // winner_pred is the bit that is being shifted out in this transition
            unsigned int winner_pred, winner_distance;
            if (zero_errors < one_errors) {
                winner_pred = 0;
                winner_distance = zero_errors;
            } else {
                winner_pred = 1;
                winner_distance = one_errors;
            }

            uint64_t history = conv->winners[j];

            // make space in the history for this row for our new bit of information
            history = (history << 1) & winner_keep_mask;

            // which predecessor won?
            history |= winner_pred;

            conv->winners[j] = history;

            // record which of the two won and update aggregate distance for this state
            // here we perform j shifted up by one with low bit clear and low bit set
            // both of these new states share a winning predecessor
            for (shift_register_t lowbit = 0; lowbit < 2; lowbit++) {
                shift_register_t newstate = (j << 1) + lowbit;

                // the aggregated error metric for this state at this time slice is the sum of
                //   the errors at this transition and the aggregated error metric of the best path leading here
                unsigned int newerror = distance(conv->table[newstate], out) + winner_distance;
                thiserrors[newstate] = newerror;

                // now track the global best path across all states for this time slice
                // we'll backtrack from the best one and decide on our next message bit
                if (__builtin_expect(newerror < leasterror, 0)) {
                    bestpath = newstate;
                    leasterror = newerror;
                }
            }
        }

        if (winner_len < winner_cap) {
            // second warmup phase (warmup history)
            // no output yet
            winner_len++;
            continue;
        }

        // warmup finished: steady state
        // declare a winner input bit for oldest set of states in history
        // take our least aggregated error for current set and walk back
        uint8_t nextbit;
        shift_register_t path = bestpath;
        if ((bestpath >> 1) == (best_path_history & best_path_mask)) {
            // our tracked path is still winning
            nextbit = (best_path_history & ((uint64_t)1 << (winner_len + conv->order - 2))) ? highbit : 0;
            best_path_history <<= 1;
            best_path_history |= bestpath & 1;
        } else {
            best_path_history = bestpath;
            for (size_t j = 0; j < winner_len; j++) {
                uint64_t winner_select_mask = ((uint64_t)1 << j);
                // we're walking backwards from what the work we did before
                // so, we'll shift high order bits in
                // the path will cross multiple different shift register states, and we determine
                //   which state by going backwards one time slice at a time
                path >>= 1;
                if (conv->winners[path] & winner_select_mask) {
                    nextbit = highbit;
                    best_path_history |= ((uint64_t)1 << (j + conv->order));
                } else {
                    nextbit = 0;
                }
                path |= nextbit;
            }
        }
        decoded_byte |= (nextbit) ? 1 : 0;
        decoded_length++;
        if (decoded_length == 8) {
            msg[decoded_index] = decoded_byte;
            decoded_index++;
            decoded_length = 0;
            decoded_byte = 0;
        } else {
            decoded_byte <<= 1;
        }
    }

    // flush out -- todo, separate this out
    // we know we padded to 0 on close

    // we will always have snarfed the highest order bit, so don't flush it again
    winner_len = (winner_len == winner_cap) ? winner_cap - 1 : winner_len;

    // flush remaining winners
    while (winner_len > 0) {
        uint8_t nextbit;
        shift_register_t path = bestpath;
        for (size_t j = 0; j < winner_len; j++) {
            uint64_t winner_select_mask = ((uint64_t)1 << j);
            path >>= 1;
            nextbit = (conv->winners[path] & winner_select_mask) ? highbit : 0;
            path |= nextbit;
        }
        decoded_byte |= (nextbit) ? 1 : 0;
        decoded_length++;
        if (decoded_length == 8) {
            msg[decoded_index] = decoded_byte;
            decoded_index++;
            decoded_length = 0;
            decoded_byte = 0;
        } else {
            decoded_byte <<= 1;
        }
        winner_len--;
    }

    return decoded_index;
}

int main() {
    srand(time(NULL));

    correct_conv *conv = correct_conv_create(2, 7, r12k7);

    char *msg = "Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World! Hello, World!";
    size_t enclen = correct_conv_encode_len(conv, strlen(msg));
    uint8_t *encoded = malloc(sizeof(uint8_t) * enclen);
    char *msgout = calloc(1, 256);

    void *vit = create_viterbi27(8 * strlen(msg));
    unsigned char *encodedbits = malloc(sizeof(char) * enclen * 8);
    unsigned char *vitdecoded = malloc(sizeof(char) * strlen(msg));
    for (size_t j = 0; j < 2000; j++) {
        printf("iteration: %zu\n", j);
        correct_conv_encode(conv, (uint8_t*)msg, strlen(msg), encoded);
        float p_err = 0.03;
        uint8_t mask = 0x80;
        unsigned int flipped = 0;
        for (size_t i = 0; i < 8 * enclen; i++) {
            if ((float)rand()/RAND_MAX > (1 - p_err)) {
                flipped++;
                encoded[i/8] ^= mask;
            }
            mask >>= 1;
            if (!mask) {
                mask = 0x80;
            }
        }
        unsigned char cmask = 0x80;
        for (size_t i = 0; i < enclen * 8; i++) {
            encodedbits[i] = (encoded[i/8] & cmask) ? 255 : 0;
            cmask >>= 1;
            if (!cmask) {
                cmask = 0x80;
            }
        }
        printf("flipped %d out of %lu bits\n", flipped, 8 * enclen);
        size_t declen = correct_conv_decode(conv, encoded, enclen, (uint8_t*)msgout);
        init_viterbi27(vit, 0);
        update_viterbi27_blk(vit, encodedbits, 8*strlen(msg)+6);
        chainback_viterbi27(vit, vitdecoded, 8*strlen(msg), 0);
        printf("decoded message matches: correct: ");
        if (strncmp(msg, msgout, strlen(msg))) {
            printf("FALSE");
        } else {
            printf("TRUE");
        }
        printf(" libfec: ");
        if (strncmp(msg, (char*)vitdecoded, strlen(msg))) {
            printf("FALSE");
        } else {
            printf("TRUE");
        }
        printf("\n");
    }
    free(encoded);
    free(msgout);
    delete_viterbi27(vit);
    free(encodedbits);
    free(vitdecoded);
    correct_conv_destroy(conv);
    return 0;
}
