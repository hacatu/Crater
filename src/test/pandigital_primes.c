//This test is based on Project Euler 41
#include "crater/container.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crater/vec.h>
#include <crater/hash.h>

uint64_t powmod(uint64_t b, uint64_t e, uint64_t n){
	uint64_t r = 1;
	b %= n;
	while(e){
		if(e&1){
			r = (uint64_t)((unsigned __int128)r*(unsigned __int128)b%(unsigned __int128)n);
		}
		e >>= 1;
		b = (uint64_t)((unsigned __int128)b*(unsigned __int128)b%(unsigned __int128)n);
	}
	return r;
}

bool is_prime_dmr(uint64_t n){
	static const uint64_t DMR_PRIMES[2] = {31, 73};//sufficient for numbers < 9080191
	static const uint64_t DMR_PRIMES_C = 2;
	uint64_t s, d;//s, d | 2^s*d = n - 1
	if(n%2 == 0){
		return n == 2;
	}
	--n;
	s = __builtin_ctzll(n);
	d = n>>s;
	++n;
	for(uint64_t i = 0, a, x; i < DMR_PRIMES_C; ++i){
		a = DMR_PRIMES[i];
		if(a >= n){
			break;
		}
		x = powmod(a, d, n);
		if(x == 1 || x == n - 1){
			goto CONTINUE_WITNESSLOOP;
		}
		for(a = 0; a < s - 1; ++a){
			x = powmod(x, 2, n);
			if(x == 1){
				return 0;
			}
			if(x == n - 1){
				goto CONTINUE_WITNESSLOOP;
			}
		}
		return 0;
		CONTINUE_WITNESSLOOP:;
	}
	return 1;
}

void process_digitperm(const cr8r_vec *_self, const cr8r_vec_ft *ft, void *_data){
	uint64_t n = 0;
	cr8r_vec *self = (cr8r_vec*)_self;
	self->len = 7;
	for(int64_t i = 1; i <= 7; ++i){
		n = 10*n + *(uint64_t*)cr8r_vec_getx(self, ft, -i);
	}
	self->len = 6;
	int status;
	cr8r_hash_insert(ft->base.data, &cr8r_htft_u64_void, &n, &status);
	if(status != 1){
		fprintf(stderr, "\e[1;31mERROR: Could not insert into hash table\e[0m\n");
	}
}

// An n-pandigital number (in base 10) is one which contains the
// digits from 1 to n once each.  Clearly, n-pandigital primes
// must end in 1, 3, 7, or 9.  We can also notice that the sums of
// the numbers {1..2}, {1..3}, {1..5}, {1..6}, {1..8}, and {1..9}
// are all odd, so only 1, 4, and 7 pandigital numbers can
// be coprime to 3.  We can then check that 1, the only 1 pandigital
// number, is not prime.  There are 12 4 pandigital numbers, of which
// 4 are prime: 1423, 2143, 2341, and 4231.  So the largest n-pandigital
// prime is either 4231 if there are no 7-pandigital primes, or
// the largest 7-pandigital prime.
// There are 3*6! = 2160 7-pandigital numbers that end in 1, 3, or 7.
// On the other hand, there are about 548038 7 digit prime numbers.
// So it makes the most sense to iterate over 7-pandigital numbers
// and check for primality, rather than iterating over 7 digit prime
// numbers and checking for pandigitalness.  Though it makes the most
// sense to prune out composite numbers while iterating over 7-pandigital
// numbers, we will use a hash table in the middle.
//
// The entire purpose of this test is to test hash table deletion.
bool gen_digitperms(cr8r_hashtbl_t *self){
	uint64_t _digitperm[7] = {7, 6, 5, 4, 2, 1, 3};
	cr8r_vec digitperm = {.buf = _digitperm, .len = 6, .cap = 7};
	cr8r_vec_ft ft = {
		.base.data = self,
		.base.size = sizeof(uint64_t),
		.new_size = cr8r_default_new_size,
		.resize = cr8r_default_resize_pass,
		.cmp = cr8r_default_cmp_u64,
		.swap = cr8r_default_swap};
	cr8r_vec_forEachPermutation(&digitperm, &ft, process_digitperm, NULL);
	for(uint64_t i = 0; i < 7; ++i){
		_digitperm[i] = 7 - i;
	}
	cr8r_vec_forEachPermutation(&digitperm, &ft, process_digitperm, NULL);
	for(uint64_t i = 0; i < 7; ++i){
		_digitperm[i] = (6 - i) ?: 7;
	}
	cr8r_vec_forEachPermutation(&digitperm, &ft, process_digitperm, NULL);
	return 1;
}

int main(){
	fprintf(stderr, "\e[1;34mSearching for largest n-pandigital prime...\e[0m\n");
	cr8r_hashtbl_t ht, ht1;
	if(!cr8r_hash_init(&ht, &cr8r_htft_u64_void, 6000)){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate hash table!\e[0m\n");
		exit(1);
	}
	if(!cr8r_hash_init(&ht1, &cr8r_htft_u64_void, 6000)){
		cr8r_hash_destroy(&ht, &cr8r_htft_u64_void);
		fprintf(stderr, "\e[1;31mERROR: Could not allocate hash table!\e[0m\n");
		exit(1);
	}
	if(!gen_digitperms(&ht)){
		exit(1);
	}
	fprintf(stderr, ht.full == 2160 ? "\e[1;32mFound %"PRIu64" 7-pandigital numbers...\e[0m\n" :
		"\e[1;31mFound wrong number of 7-pandigital numbers (%"PRIu64")\e[0m\n", ht.full);
	uint64_t max_panprime = 4231;
	for(uint64_t *it = cr8r_hash_next(&ht, &cr8r_htft_u64_void, NULL); it; it = cr8r_hash_next(&ht, &cr8r_htft_u64_void, it)){
		if(is_prime_dmr(*it)){
			int status;
			cr8r_hash_insert(&ht1, &cr8r_htft_u64_void, it, &status);
			if(status != 1){
				fprintf(stderr, "\e[1;31mERROR: Could not insert into hash table\e[0m\n");
			}
			if(*it > max_panprime){
				max_panprime = *it;
			}
		}else{
			cr8r_hash_delete(&ht, &cr8r_htft_u64_void, it);
		}
	}
	if(ht.full != ht1.full){
		fprintf(stderr, "\e[1;31mERROR: %"PRIu64" 7-pandigital primes in ht but %"PRIu64" in ht1!\e[0m\n", ht.full, ht1.full);
		cr8r_hash_destroy(&ht, &cr8r_htft_u64_void);
		cr8r_hash_destroy(&ht1, &cr8r_htft_u64_void);
		exit(1);
	}else{
		cr8r_hash_destroy(&ht1, &cr8r_htft_u64_void);
		fprintf(stderr, "\e[1;33mFound %"PRIu64" 7-pandigital primes\e[0m\n", ht.full);
		cr8r_hash_destroy(&ht, &cr8r_htft_u64_void);
		fprintf(stderr, "%sLargest n-pandigital prime is %"PRIu64"\e[0m\n", max_panprime == 7652413 ? "\e[1;32m" : "\e[1;31m", max_panprime);
	}
}

