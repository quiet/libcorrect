#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "rs_tester.h"
#include "rs_tester_fec.h"

void print_test_type(size_t block_length, size_t message_length,
                     size_t num_errors, size_t num_erasures) {
    printf(
        "testing reed solomon block length=%zu, message length=%zu, "
        "errors=%zu, erasures=%zu...",
        block_length, message_length, num_errors, num_erasures);
}

void fail_test() {
    printf("FAILED\n");
    exit(1);
}

void pass_test() { printf("PASSED\n"); }

void run_tests(correct_reed_solomon *rs, void *fec_rs, rs_testbench *testbench,
               size_t block_length, size_t test_msg_length, size_t num_errors,
               size_t num_erasures, size_t num_iterations) {
    // run both ways, correct->fec and fec->correct
    rs_test test;
    test.encode = rs_correct_encode;
    test.encoder = rs;
    test.decode = rs_fec_decode;
    test.decoder = fec_rs;

    print_test_type(block_length, test_msg_length, num_errors, num_erasures);
    for (size_t i = 0; i < num_iterations; i++) {
        rs_test_run run = test_rs_errors(&test, testbench, test_msg_length,
                                         num_errors, num_erasures);
        if (!run.output_matches) {
            fail_test();
        }
    }

    test.encode = rs_fec_encode;
    test.encoder = fec_rs;
    test.decode = rs_correct_decode;
    test.decoder = rs;
    for (size_t i = 0; i < num_iterations; i++) {
        rs_test_run run = test_rs_errors(&test, testbench, test_msg_length,
                                         num_errors, num_erasures);
        if (!run.output_matches) {
            fail_test();
        }
    }
    pass_test();
}

void benchmark(rs_test test, rs_testbench *testbench, size_t block_length,
               size_t test_msg_length, size_t num_errors, size_t num_erasures,
               size_t num_iterations) {
    uint64_t encoder_elapsed_micro = 0;
    uint64_t decoder_elapsed_micro = 0;
    for (size_t i = 0; i < num_iterations; i++) {
        rs_test_run run = test_rs_errors(&test, testbench, test_msg_length,
                                         num_errors, num_erasures);
        encoder_elapsed_micro += run.encoder_elapsed.tv_usec;
        encoder_elapsed_micro += 1000000 * run.encoder_elapsed.tv_sec;

        decoder_elapsed_micro += run.decoder_elapsed.tv_usec;
        decoder_elapsed_micro += 1000000 * run.decoder_elapsed.tv_sec;
    }

    float encoder_elapsed = encoder_elapsed_micro / 1000000.0f;
    float decoder_elapsed = decoder_elapsed_micro / 1000000.0f;

    // in mbps!
    float encoder_throughput =
        ((8 * test_msg_length * num_iterations) / encoder_elapsed) / 1000000.0f;
    float decoder_throughput =
        ((8 * test_msg_length * num_iterations) / decoder_elapsed) / 1000000.0f;

    printf(
        "block length=%zu, message length=%zu, errors=%zu, erasures=%zu, "
        "encoder throughput=%.1f Mbps, decoder throughput=%.1f Mbps\n",
        block_length, test_msg_length, num_errors, num_erasures,
        encoder_throughput, decoder_throughput);
}

int main() {
    srand(time(NULL));

    size_t block_length = 255;
    size_t min_distance = 32;
    size_t message_length = block_length - min_distance;

    size_t pad_length;
    void *fec_rs;

    correct_reed_solomon *rs = correct_reed_solomon_create(
        correct_rs_primitive_polynomial_ccsds, 1, 1);
    rs_testbench *testbench = rs_testbench_create(block_length, min_distance);

    pad_length = message_length / 2;
    fec_rs = init_rs_char(8, correct_rs_primitive_polynomial_ccsds, 1, 1, 32,
                          pad_length);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              0, 0, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              min_distance / 2, 0, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              0, min_distance, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              min_distance / 4, min_distance / 2, 20000);
    free_rs_char(fec_rs);

    pad_length = 0;
    fec_rs = init_rs_char(8, correct_rs_primitive_polynomial_ccsds, 1, 1, 32,
                          pad_length);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              0, 0, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              min_distance / 2, 0, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              0, min_distance, 20000);
    run_tests(rs, fec_rs, testbench, block_length, message_length - pad_length,
              min_distance / 4, min_distance / 2, 20000);
    free_rs_char(fec_rs);

    printf("benchmarking\n");
    rs_test test;

    pad_length = message_length / 2;
    test.encode = rs_correct_encode;
    test.encoder = rs;
    test.decode = rs_correct_decode;
    test.decoder = rs;
    printf("benchmarking libcorrect ");
    benchmark(test, testbench, block_length, message_length - pad_length, 0, 0,
              200000);
    printf("benchmarking libcorrect ");
    benchmark(test, testbench, block_length, message_length - pad_length,
              min_distance / 2, 0, 200000);
    printf("benchmarking libcorrect ");
    benchmark(test, testbench, block_length, message_length - pad_length, 0,
              min_distance, 200000);
    printf("benchmarking libcorrect ");
    benchmark(test, testbench, block_length, message_length - pad_length,
              min_distance / 4, min_distance / 2, 200000);

    pad_length = 0;
    test.encode = rs_correct_encode;
    test.encoder = rs;
    test.decode = rs_correct_decode;
    test.decoder = rs;
    printf("benchmarking libcorrect ");
    benchmark(test, testbench, block_length, message_length - pad_length, 0, 0,
              200000);
    printf("benchmarking libcorrect ");
    benchmark(test, testbench, block_length, message_length - pad_length,
              min_distance / 2, 0, 200000);
    printf("benchmarking libcorrect ");
    benchmark(test, testbench, block_length, message_length - pad_length, 0,
              min_distance, 200000);
    printf("benchmarking libcorrect ");
    benchmark(test, testbench, block_length, message_length - pad_length,
              min_distance / 4, min_distance / 2, 200000);

    pad_length = message_length / 2;
    fec_rs = init_rs_char(8, correct_rs_primitive_polynomial_ccsds, 1, 1, 32,
                          pad_length);
    test.encode = rs_fec_encode;
    test.encoder = fec_rs;
    test.decode = rs_fec_decode;
    test.decoder = fec_rs;
    printf("benchmarking libfec ");
    benchmark(test, testbench, block_length, message_length - pad_length, 0, 0,
              200000);
    printf("benchmarking libfec ");
    benchmark(test, testbench, block_length, message_length - pad_length,
              min_distance / 2, 0, 200000);
    printf("benchmarking libfec ");
    benchmark(test, testbench, block_length, message_length - pad_length, 0,
              min_distance, 200000);
    printf("benchmarking libfec ");
    benchmark(test, testbench, block_length, message_length - pad_length,
              min_distance / 4, min_distance / 2, 200000);
    free_rs_char(fec_rs);

    pad_length = 0;
    fec_rs = init_rs_char(8, correct_rs_primitive_polynomial_ccsds, 1, 1, 32,
                          pad_length);
    test.encode = rs_fec_encode;
    test.encoder = fec_rs;
    test.decode = rs_fec_decode;
    test.decoder = fec_rs;
    printf("benchmarking libfec ");
    benchmark(test, testbench, block_length, message_length - pad_length, 0, 0,
              200000);
    printf("benchmarking libfec ");
    benchmark(test, testbench, block_length, message_length - pad_length,
              min_distance / 2, 0, 200000);
    printf("benchmarking libfec ");
    benchmark(test, testbench, block_length, message_length - pad_length, 0,
              min_distance, 200000);
    printf("benchmarking libfec ");
    benchmark(test, testbench, block_length, message_length - pad_length,
              min_distance / 4, min_distance / 2, 200000);
    free_rs_char(fec_rs);

    rs_testbench_destroy(testbench);
    correct_reed_solomon_destroy(rs);

    printf("test passed\n");
    return 0;
}
