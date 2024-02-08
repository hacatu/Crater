#include "crater/container.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crater/prand.h>

const uint64_t gs[4] = {1, (1ull << 63) + 3, (1ull << 63) - 1, -1};

CR8R_ATTR_NO_SAN("unsigned-integer-overflow")
int main(){
	fprintf(stderr, "\e[1;34mTesting cr8r_prng_log_mul_t64 on 1000 random integers\e[0m\n");
	cr8r_prng *prng = cr8r_prng_init_xoro(0x92f71af767c63704);
	uint64_t num_even_or_one = 0, num_odd_correct = 0, num_odd_failed = 0, num_odd_incorrect = 0;
	for(uint64_t i = 0; i < 1000; ++i){
		uint64_t h = cr8r_prng_get_u64(prng);
		uint64_t x = cr8r_prng_log_mod_t64(h);
		if(!x){
			if((h&1) && h != 1){
				fprintf(stderr, "\e[1;31mlog_mod_t64(%"PRIu64") returned zero on an odd argument besides 1\e[0m\n", h);
				++num_odd_failed;
			}else{
				++num_even_or_one;
			}
		}else{
			uint64_t g = gs[x >> 62];
			x &= ~0ull >> 2;
			if(h != g*cr8r_pow_u64(3, x)){
				fprintf(stderr, "\e[1;31mg,x = log_mod_t64(h=%"PRIu64") does not actually satisfy h=g**x mod 2**64\e[0m\n", h);
				++num_odd_incorrect;
			}else{
				++num_odd_correct;
			}
		}
	}
	free(prng);
	if(!num_odd_failed && !num_odd_incorrect){
		fprintf(stderr, "\e[1;32mSUCCESS: ");
	}else{
		fprintf(stderr, "\e[1;31mFAILED: ");
	}
	fprintf(stderr, "%"PRIu64" even or 1 + %"PRIu64" odd h processed correctly,\n"
	        "%"PRIu64" odd failed + %"PRIu64" odd incorrect\e[0m\n",
			num_even_or_one, num_odd_correct, num_odd_failed, num_odd_incorrect);
}

