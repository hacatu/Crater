#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <crater/vec.h>

int main(){
	uint64_t p = 9973;
	fprintf(stderr, "\e[1;34mTesting miscellaneous vec methods...\e[0m\n");
	cr8r_vec nums_asc, nums_shuf;
	if(!cr8r_vec_init(&nums_shuf, &cr8r_vecft_u64, p)){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate vector!\e[0m");
		exit(1);
	}
	uint64_t g = 0;
	cr8r_vec_pushr(&nums_shuf, &cr8r_vecft_u64, &g);
	// 11 is a generator modulo 9973 so this generates all numbers besides 0 exactly once
	for(g = 11;; g = g*11%p){
		cr8r_vec_pushr(&nums_shuf, &cr8r_vecft_u64, &g);
		if(g == 1){
			break;
		}
	}
	cr8r_vec_sorted(&nums_asc, &nums_shuf, &cr8r_vecft_u64);
	g = 2*p;
	for(uint64_t i = 0; i < p; ++i){
		if(!cr8r_vec_containss(&nums_asc, &cr8r_vecft_u64, &i)){
			--g;
		}
		if(!cr8r_vec_contains(&nums_shuf, &cr8r_vecft_u64, &i)){
			--g;
		}
	}
	fprintf(stderr, g == 2*p ? "\e[1;32mSuccess: passed %"PRIu64"/%"PRIu64" %s\e[0m\n" :
		"\e[1;31mFailed: passed %"PRIu64"/%"PRIu64" %s\e[0m\n", g, 2*p, "scrambled/sorted contains checks");
	cr8r_vec_popl(&nums_asc, &cr8r_vecft_u64, &g);
	if(g){
		fprintf(stderr, "\e[1;31mERROR: Popped wrong value from beginning of sorted vector\e[0m\n");
	}
	cr8r_vec_popl(&nums_shuf, &cr8r_vecft_u64, &g);
	if(g){
		fprintf(stderr, "\e[1;31mERROR: Popped wrong value from beginning of scrambled vector\e[0m\n");
	}
	g = (uint64_t)cr8r_vec_foldl(&nums_shuf, &cr8r_vecft_u64, (void*)0, cr8r_default_acc_sum_u64);
	uint64_t sum = p*(p - 1)/2;
	if(g != sum){
		fprintf(stderr, "\e[1;31mERROR: Wrong foldl linear checksum for scrambled vector\e[0m\n");
	}
	g = (uint64_t)cr8r_vec_foldr(&nums_shuf, &cr8r_vecft_u64, (void*)0, cr8r_default_acc_sum_u64);
	if(g != sum){
		fprintf(stderr, "\e[1;31mERROR: Wrong foldr linear checksum for scrambled vector\e[0m\n");
	}
	g = (uint64_t)cr8r_vec_foldl(&nums_asc, &cr8r_vecft_u64, (void*)0, cr8r_default_acc_sum_u64);
	if(g != sum){
		fprintf(stderr, "\e[1;31mWrong foldl linear checksum for sorted vector\e[0m\n");
	}
	g = (uint64_t)cr8r_vec_foldr(&nums_asc, &cr8r_vecft_u64, (void*)0, cr8r_default_acc_sum_u64);
	if(g != sum){
		fprintf(stderr, "\e[1;31mERROR: Wrong foldr linear checksum for sorted vector\e[0m\n");
	}
	uint64_t sum2 = p*(p - 1)*(2*p - 1)/6;
	uint64_t acc[3] = {0, 2, sum2 + 1};
	cr8r_vec_foldr(&nums_shuf, &cr8r_vecft_u64, acc, cr8r_default_acc_sumpowmod_u64);
	if(acc[0] != sum2){
		fprintf(stderr, "\e[1;31mERROR: Wrong foldr quadratic checksum for scrambled vector\e[0m\n");
	}
	cr8r_vec_delete(&nums_asc, &cr8r_vecft_u64);
	cr8r_vec_delete(&nums_shuf, &cr8r_vecft_u64);
}

