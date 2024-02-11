// This test case is based on Project Euler problem 7
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crater/heap.h>
#include <crater/sla.h>
#include <crater/pheap.h>

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
uint64_t nth_prime_heap(uint64_t n){
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

uint64_t nth_prime_pheap(uint64_t n){
	cr8r_sla sla [[gnu::cleanup(cr8r_sla_delete)]] = {};
	typedef struct{
		generator g;
		cr8r_pheap_node n;
	} gnode;
	if(!cr8r_sla_init(&sla, sizeof(gnode), n)){
		return 0;
	}
	cr8r_pheap_ft ft = {
		.base.size = offsetof(gnode, n),
		.base.data = &sla,
		.alloc = cr8r_default_alloc_sla,
		.cmp = cr8r_default_cmp_u64,
		.free = cr8r_default_free_sla
	};
	cr8r_pheap_node *generators = cr8r_pheap_new(&(generator){4, 2}, &ft, NULL, NULL, NULL);
	if(!generators){
		return 0;
	}
	uint64_t p = 2, i = 0;
	while(1){
		cr8r_pheap_node *tmp;
		gnode *top = cr8r_pheap_top(generators, &ft);
		if(p < top->g.m){
			if(++i == n){
				cr8r_pheap_delete(&top->n, &ft);
				return p;
			}
			tmp = cr8r_pheap_push(generators, &(generator){p*p, p}, &ft);
			if(!tmp){
				return 0;
			}
			generators = tmp;
			++p;
			continue;
		}else if(p == top->g.m){
			++p;
		}
		// notice that if we get here, we did not modify the pheap so top is still a valid pointer to the root element
		top->g.m += top->g.p;
		tmp = cr8r_pheap_pop(&generators, &ft);
		*tmp = (cr8r_pheap_node){};
		generators = cr8r_pheap_meld(generators, tmp, &ft);
	}
}

int main(){
	uint64_t p = nth_prime_heap(10001);
	if(!p){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate space for prime heap!\e[0m\n");
		exit(1);
	}else if(p == 104743){
		fprintf(stderr, "\e[1;32mSuccess: Found 10001st prime number: %"PRIu64"\e[0m\n", p);
	}else{
		fprintf(stderr, "\e[1;31mFailed: Found wrong 10001st prime number: %"PRIu64"\e[0m\n", p);
	}
	p = nth_prime_pheap(10001);
	if(!p){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate space for prime pheap!\e[0m\n");
		exit(1);
	}else if(p == 104743){
		fprintf(stderr, "\e[1;32mSuccess: Found 10001st prime number: %"PRIu64"\e[0m\n", p);
	}else{
		fprintf(stderr, "\e[1;31mFailed: Found wrong 10001st prime number: %"PRIu64"\e[0m\n", p);
	}
}

