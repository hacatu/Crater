#include "crater/prand.h"
#include "crater/vec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crater/heap.h>

static bool hupify(cr8r_vec *self, cr8r_vec_ft *ft, int ord){
	for(uint64_t i = 1; i < self->len ; ++i){
		cr8r_heap_sift_up(self, ft, self->buf + i*ft->base.size, ord);
	}
	for(uint64_t i = 1; i < self->len; ++i){
		uint64_t j = (i - 1) >> 1;
		if(ord*ft->cmp(&ft->base, self->buf + i*ft->base.size, self->buf + j*ft->base.size) > 0){
			return false;
		}
	}
	return true;
}

int main(){
	cr8r_prng *prng = cr8r_prng_init_lfg_m(0x03edef6607cfc2c6);
	cr8r_vec nums;
	cr8r_vec_init(&nums, &cr8r_vecft_u64, 100);
	uint64_t trials = 1000, passed = 0;
	for(uint64_t i = 0; i < trials; ++i){
		cr8r_prng_get_bytes(prng, nums.cap*sizeof(uint64_t), nums.buf);
		nums.len = nums.cap;
		if(hupify(&nums, &cr8r_vecft_u64, 1)){
			++passed;
		}
	}
	cr8r_vec_delete(&nums, &cr8r_vecft_u64);
	free(prng);
	fprintf(stderr, "%s: Passed %"PRIu64"/%"PRIu64" trials!\e[0m\n", passed == trials ? "\e[1;32mSuccess" : "\e[1;31mFailed", passed, trials);
}

