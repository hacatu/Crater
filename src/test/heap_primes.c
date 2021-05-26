// This test case is based on Project Euler problem 7
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crater/heap.h>

typedef struct{
	uint64_t m, p;
} generator;

// Uses a min heap to store multiples of all primes encountered so far,
// together with a counter p.  Each multiple in the heap is
// associated with a prime.
// If p < top, p is prime, add the pair (multiple = p*p, prime = p) to the heap,
// and increment p.
// If p == top, p is composite, increment p, set the top (multiple, prime) to
// (multiple + prime, prime), and sift down the top element.
// If p > top, set the top (multiple, prime) to
// (multiple + prime, prime), and sift down the top element.
// This is just the seive of eratosthenes but using a heap to merge the streams of
// prime multiples so they can be removed from the stream of increasing integers,
// rather than using a fixed bitarray.
uint64_t nth_prime(uint64_t n){
	cr8r_vec generators;
	cr8r_vec_ft vecft = cr8r_vecft_u64;
	vecft.base.size = sizeof(generator);
	if(!cr8r_vec_init(&generators, &vecft, n)){
		return 0;
	}
	cr8r_vec_pushr(&generators, &vecft, &(generator){4, 2});
	uint64_t p = 2, i = 0;
	while(1){
		generator *top = (generator*)cr8r_heap_top(&generators, &vecft);
		if(p < top->m){
			if(++i == n){
				cr8r_vec_delete(&generators, &vecft);
				return p;
			}
			cr8r_heap_push(&generators, &vecft, &(generator){.m=p*p, .p=p}, -1);
			++p;
			continue;
		}else if(p == top->m){
			++p;
		}
		top->m += top->p;
		cr8r_heap_sift_down(&generators, &vecft, top, -1);
	}
}

int main(){
	uint64_t p = nth_prime(10001);
	if(!p){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate space for prime sieve!\e[0m\n");
		exit(1);
	}else if(p == 104743){
		fprintf(stderr, "\e[1;32mSuccess: Found 10001st prime number: %"PRIu64"\e[0m\n", p);
	}else{
		fprintf(stderr, "\e[1;31mFailed: Found wrong 10001st prime number: %"PRIu64"\e[0m\n", p);
	}
}

