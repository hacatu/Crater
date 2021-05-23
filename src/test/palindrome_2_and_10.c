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

bool is_even_popcount(const void *_n){
	uint64_t n = *(const uint64_t*)_n;
	return !(__builtin_popcountll(n)&1);
}

bool is_odd_popcount(const void *_n){
	uint64_t n = *(const uint64_t*)_n;
	return __builtin_popcountll(n)&1;
}

typedef struct{
	cr8r_vec *out, *in;
} job;

void *worker(void *_job){
	job task = *(job*)_job;
	if(!cr8r_vec_init(task.in, &cr8r_vecft_u64, 512)){
		return NULL;
	}
	for(uint64_t i = 1;; ++i){
		uint64_t pal2 = i;
		for(uint64_t t = i; t; t >>= 1){
			pal2 = (pal2 << 1) | (t & 1);
		}
		if(pal2 >= 1000000){
			break;
		}
		cr8r_vec_pushr(task.in, &cr8r_vecft_u64, &pal2);
	}
	if(!cr8r_vec_filtered(task.out, task.in, &cr8r_vecft_u64, is_pal10)){
		return NULL;
	}
	cr8r_vec_delete(task.in, &cr8r_vecft_u64);
	return _job;//non-null return value
}

int main(){
	fprintf(stderr, "\e[1;34mFinding all numbers < 1 million that are palindromes in base 2 and 10...\e[0m\n");
	cr8r_vec pals_2_odd = {}, pals_2_even = {}, pals_both_odd = {}, pals_both_even = {}, pals_both = {};
	pthread_t tid;
	job task = {.out=&pals_both_even, .in=&pals_2_even};
	pthread_create(&tid, NULL, worker, &task);
	if(!cr8r_vec_init(&pals_2_odd, &cr8r_vecft_u64, 512)){
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
		cr8r_vec_pushr(&pals_2_odd, &cr8r_vecft_u64, &pal2);
	}
	if(!cr8r_vec_filtered(&pals_both_odd, &pals_2_odd, &cr8r_vecft_u64, is_pal10)){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate vector!\e[0m\n");
		exit(1);
	}
	cr8r_vec_delete(&pals_2_odd, &cr8r_vecft_u64);
	void *res;
	pthread_join(tid, &res);
	if(!res){
		fprintf(stderr, "\e[1;31mERROR: Worker thread failed!\e[0m\n");
		exit(1);
	}
	if(
		!cr8r_vec_all(&pals_both_even, &cr8r_vecft_u64, is_even_popcount) ||
		cr8r_vec_any(&pals_both_even, &cr8r_vecft_u64, is_odd_popcount)
	){
		fprintf(stderr, "\e[1;31mERROR: Even base 2 length palindrome has odd popcount\e[0m\n");
	}
	if(
		cr8r_vec_all(&pals_both_odd, &cr8r_vecft_u64, is_even_popcount) ||
		!cr8r_vec_any(&pals_both_odd, &cr8r_vecft_u64, is_odd_popcount)
	){
		fprintf(stderr, "\e[1;31mERROR: All odd base 2 length palindrome have even popcounts\e[0m\n");
	}
	if(!cr8r_vec_combine(&pals_both, &pals_both_odd, &pals_both_even, &cr8r_vecft_u64)){
		fprintf(stderr, "\e[1;31mERROR: Could not combine vectors!\e[0m\n");
		exit(1);
	}
	cr8r_vec_delete(&pals_both_odd, &cr8r_vecft_u64);
	cr8r_vec_delete(&pals_both_even, &cr8r_vecft_u64);
	uint64_t s = 0;
	for(uint64_t i = 0; i < pals_both.len; ++i){
		s += *(uint64_t*)cr8r_vec_get(&pals_both, &cr8r_vecft_u64, i);
	}
	cr8r_vec_delete(&pals_both, &cr8r_vecft_u64);
	fprintf(stderr, s == 872187 ? "\e[1;32mGot sum %"PRIu64"\e[0m\n\e[1;32mSuccess: passed 1/1 tests\e[0m\n"
		: "\e[1;31mERROR: Got wrong sum %"PRIu64"\e[0m\n\e[1;31mFailed: passed 0/1 tests\e[0m\n", s);
}

