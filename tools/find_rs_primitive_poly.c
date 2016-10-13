#include "correct/reed-solomon.h"

// visit all of the elements from the poly
bool trypoly(field_operation_t poly, field_logarithm_t *log) {
    memset(log, 0, 256);
    field_operation_t element = 1;
    log[0] = (field_logarithm_t)0;
    for (field_operation_t i = 1; i < 256; i++) {
        element = element * 2;
        element = (element > 255) ? (element ^ poly) : element;
        if (log[element] != 0) {
            return false;
        }
        log[element] = (field_logarithm_t)i;
    }
    return true;
}

int main() {
    field_logarithm_t *log = malloc(256 * sizeof(field_logarithm_t));
    for (field_operation_t i = 0x100; i < 0x200; i++) {
        if (trypoly(i, log)) {
            printf("0x%x valid: ", i);
            field_operation_t poly = i;
            int power = 8;
            while(poly) {
                if (poly & 0x100) {
                    if (power > 1) {
                        printf("x^%d", power);
                    } else if (power) {
                        printf("x");
                    } else {
                        printf("1");
                    }
                    if (poly & 0xff) {
                        printf(" + ");
                    }
                }
                power--;
                poly <<= 1;
                poly &= 0x1ff;
            }
            printf("\n");
        }
    }
    free(log);
    return 0;
}
