// This test case is based on Project Euler problem 36
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>

#include <crater/vec.h>

bool is_pal10(const void *_n){
	uint64_t n = *(const uint64_t*)_n;
	uint64_t fwd = n, rev = 0;
	for(; n; n /= 10){
		rev = rev*10 + n%10;
	}
	return fwd == rev;
}

int main(){
	fprintf(stderr, "\e[1;34mFinding all numbers < 1 million that are palindromes in base 2 and 10...\e[0m\n");
	cr8r_vec pals2 = {}, pals_both = {};
	if(!cr8r_vec_init(&pals2, &cr8r_vecft_u64, 1024)){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate vector!\e[0m\n");
		exit(1);
	}
	for(uint64_t i = 1;; ++i){
		uint64_t pal2 = i;
		for(uint64_t t = i >> 1; t; t >>= 1){
			pal2 = (pal2 << 1) | (t & 1);
		}
		if(pal2 >= 1000000){
			break;
		}
		cr8r_vec_pushr(&pals2, &cr8r_vecft_u64, &pal2);
		pal2 = i;
		for(uint64_t t = i; t; t >>= 1){
			pal2 = (pal2 << 1) | (t & 1);
		}
		if(pal2 < 1000000){
			cr8r_vec_pushr(&pals2, &cr8r_vecft_u64, &pal2);
		}
	}
	if(!cr8r_vec_filtered(&pals_both, &pals2, &cr8r_vecft_u64, is_pal10)){
		cr8r_vec_delete(&pals2, &cr8r_vecft_u64);
		fprintf(stderr, "\e[1;31mERROR: Could not allocate vector!\e[0m\n");
		exit(1);
	}
	cr8r_vec_delete(&pals2, &cr8r_vecft_u64);
	uint64_t s = 0;
	for(uint64_t i = 0; i < pals_both.len; ++i){
		s += *(uint64_t*)cr8r_vec_get(&pals_both, &cr8r_vecft_u64, i);
	}
	cr8r_vec_delete(&pals_both, &cr8r_vecft_u64);
	fprintf(stderr, s == 872187 ? "\e[1;32mGot sum %"PRIu64"\e[0m\n" : "\e[1;31mGot wrong sum %"PRIu64"\e[0m\n", s);
}

