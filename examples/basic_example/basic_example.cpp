extern "C"{
#include <libcorrect/correct.h>
}

#include <cstdint> 
int main(void)
{
        int n = 255;
        int k = 251;
	int m_m = n - k;

	correct_reed_solomon* m_rs = correct_reed_solomon_create(correct_rs_primitive_polynomial_ccsds, 1, 1, m_m);
        (void) m_rs;
        return 0;
}
