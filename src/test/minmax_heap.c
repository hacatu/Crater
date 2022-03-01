#include "crater/container.h"
#include "crater/vec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <crater/minmax_heap.h>

static int test_exm100(cr8r_vec *vec, cr8r_vec_ft *ft, cr8r_prng *prng){
	cr8r_vec lo, hi;
	cr8r_vec_init(&lo, ft, 100);
	cr8r_vec_init(&hi, ft, 100);
	int status = 1;
	for(uint64_t i = 0; i < 1000; ++i){
		cr8r_vec_shuffle(vec, ft, prng);
		cr8r_vec_clear(&lo, ft);
		cr8r_vec_clear(&hi, ft);
		for(uint64_t j = 0; j < vec->len; ++j){
			if(lo.len == 100){
				uint64_t o;
				cr8r_mmheap_pushpop_max(&lo, ft, cr8r_vec_get(vec, ft, j), &o);
			}else{
				cr8r_mmheap_push(&lo, ft, cr8r_vec_get(vec, ft, j));
			}
			if(hi.len == 100){
				uint64_t o;
				cr8r_mmheap_pushpop_min(&hi, ft, cr8r_vec_get(vec, ft, j), &o);
			}else{
				cr8r_mmheap_push(&hi, ft, cr8r_vec_get(vec, ft, j));
			}
		}
		if(*(uint64_t*)cr8r_mmheap_peek_min(&lo, ft) != 0){
			fprintf(stderr, "\e[1;31mFound invalid min on trial %"PRIu64"\e[0m\n", i);
			status = 0;
			break;
		}else if(*(uint64_t*)cr8r_mmheap_peek_max(&lo, ft) != 99){
			fprintf(stderr, "\e[1;31mFound invalid 100th smallest on trial %"PRIu64"\e[0m\n", i);
			status = 0;
			break;
		}else if(*(uint64_t*)cr8r_mmheap_peek_max(&hi, ft) != 999){
			fprintf(stderr, "\e[1;31mFound invalid max on trial %"PRIu64"\e[0m\n", i);
			status = 0;
			break;
		}else if(*(uint64_t*)cr8r_mmheap_peek_min(&hi, ft) != 900){
			fprintf(stderr, "\e[1;31mFound invalid max on trial %"PRIu64"\e[0m\n", i);
			status = 0;
			break;
		}
	}
	cr8r_vec_delete(&lo, ft);
	cr8r_vec_delete(&hi, ft);
	return status;
}

int main(){
	fprintf(stderr, "\e[1;34mTesting vector minmax heap functions\e[0m\n");
	uint64_t tested = 0, passed = 0;
	cr8r_vec vec;
	cr8r_vec_ft ft = cr8r_vecft_u64;
	cr8r_vec_init(&vec, &ft, 1000);
	vec.len = vec.cap;
	for(uint64_t i = 0; i < vec.cap; ++i){
		*(uint64_t*)cr8r_vec_get(&vec, &ft, i) = i;
	}
	cr8r_prng *prng = cr8r_prng_init_lfg_m(0x746ca0ffd40e136);

	fprintf(stderr, "\e[1;34mFinding 100th smallest element in a 1000 element array 1000x\e[0m\n");
	int status = test_exm100(&vec, &ft, prng);
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32m100th smallest test succeeded!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31m100th smallest test failed!\e[0m\n");
	}

	cr8r_vec_shuffle(&vec, &ft, prng);
	cr8r_mmheap_ify(&vec, &ft);
	++tested;
	if(*(uint64_t*)cr8r_mmheap_peek_min(&vec, &ft) != 0 || *(uint64_t*)cr8r_mmheap_peek_max(&vec, &ft) != 999){
		fprintf(stderr, "\e[1;31mheapify test failed!\e[0m\n");
	}else{
		fprintf(stderr, "\e[1;32mheapify test succeeded!\e[0m\n");
		++passed;
	}

	free(prng);
	cr8r_vec_delete(&vec, &ft);
	if(passed == tested){
		fprintf(stderr, "\e[1;32mSuccess: passed %"PRIu64"/%"PRIu64" tests\e[0m\n", passed, tested);
	}else{
		fprintf(stderr, "\e[1;31mFailed: passed %"PRIu64"/%"PRIu64" tests\e[0m\n", passed, tested);
	}
}

