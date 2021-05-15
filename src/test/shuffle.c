#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <crater/vec.h>

int main(){
	uint64_t num_trials = 1000;
	fprintf(stderr, "\e[1;34mTesting shuffling/sorting 10000 element vector 1000 times...\e[0m\n");
	cr8r_vec nums_asc, nums_shuf;
	cr8r_prng *prng = cr8r_prng_init_lfg_m(0xf21e0575bfa22604);
	if(!prng){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate prng!\e[0m\n");
		exit(1);
	}
	if(!cr8r_vec_init(&nums_asc, &cr8r_vecft_u64, 10000)){
		free(prng);
		fprintf(stderr, "\e[1;31mERROR: Could not allocate vector!\e[0m");
		exit(1);
	}
	for(uint64_t i = 0; i < 10000; ++i){
		cr8r_vec_pushr(&nums_asc, &cr8r_vecft_u64, &i);
	}
	if(!cr8r_vec_sub(&nums_shuf, &nums_asc, &cr8r_vecft_u64, 0, 10000)){
		free(prng);
		cr8r_vec_delete(&nums_asc, &cr8r_vecft_u64);
		fprintf(stderr, "\e[1;31mERROR: Could not allocate vector!\e[0m");
		exit(1);
	}
	if(cr8r_vec_cmp(&nums_shuf, &nums_asc, &cr8r_vecft_u64)){
		free(prng);
		cr8r_vec_delete(&nums_asc, &cr8r_vecft_u64);
		cr8r_vec_delete(&nums_shuf, &cr8r_vecft_u64);
		fprintf(stderr, "\e[1;31mERROR: Copyng vector failed!\e[0m");
		exit(1);
	}
	cr8r_vec_delete(&nums_shuf, &cr8r_vecft_u64);
	cr8r_vec_copy(&nums_shuf, &nums_asc, &cr8r_vecft_u64);
	uint64_t passed = num_trials;
	for(uint64_t i = 0; i < num_trials; ++i){
		cr8r_vec_shuffle(&nums_shuf, &cr8r_vecft_u64, prng);
		cr8r_vec_sort(&nums_shuf, &cr8r_vecft_u64);
		if(cr8r_vec_cmp(&nums_shuf, &nums_asc, &cr8r_vecft_u64)){
			--passed;
		}
	}
	cr8r_vec_delete(&nums_shuf, &cr8r_vecft_u64);
	free(prng);
	fprintf(stderr, passed == num_trials ?
		"\e[1;32mSuccess: passed %"PRIu64"/%"PRIu64" tests\e[0m\n" :
		"\e[1;31mFailed: passed %"PRIu64"/%"PRIu64" tests\e[0m\n", passed, num_trials
	);
	cr8r_vec_reversed(&nums_shuf, &nums_asc, &cr8r_vecft_u64);
	cr8r_vec_reverse(&nums_shuf, &cr8r_vecft_u64);
	if(cr8r_vec_cmp(&nums_shuf, &nums_asc, &cr8r_vecft_u64)){
		fprintf(stderr, "\e[1;31mERROR: vec_reversed -> vec_reverse failed!\e[0m\n");
		exit(1);
	}
	cr8r_vec_delete(&nums_asc, &cr8r_vecft_u64);
	cr8r_vec_delete(&nums_shuf, &cr8r_vecft_u64);
}

