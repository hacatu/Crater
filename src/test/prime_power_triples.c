// This test case is based on Project Euler problem 87
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <crater/vec.h>
#include <crater/hash.h>

bool sieve_primes_silly(cr8r_vec *out, uint64_t max){
	// numbers which are coprime to 2*3*5*7
	static const uint64_t wheel[] = {
		  1, 11, 13, 17, 19, 23, 29,
		 31, 37, 41, 43, 47, 53, 59,
		 61, 67, 71, 73, 79, 83, 89,
		 97,101,103,107,109,113,
		121,127,131,137,139,143,149,
		151,157,163,167,169,173,179,
		181,187,191,193,197,199,209
	};
	static const uint64_t wheel_primes[] = {
		2, 3, 5, 7
	};
	static const uint64_t wheel_size = 2*3*5*7;
	static const uint64_t wheel_spokes = 48;
	static const uint64_t wheel_primes_count = 4;
	cr8r_hashtbl_t is_prime;
	cr8r_hashtbl_ft htft_generic = {
		.base = {
			.data = NULL,
			.size = sizeof(uint64_t)
		},
		.hash = cr8r_default_hash,
		.cmp = cr8r_default_cmp,
		.add = NULL,
		.del = NULL,
		.load_factor = .5
	};
	if(!cr8r_hash_init(&is_prime, &htft_generic, max*wheel_spokes*3/wheel_size)){
		return 0;
	}
	for(uint64_t i = 0; i < wheel_spokes; ++i){
		for(uint64_t n = wheel[i]; n <= max; n += wheel_size){
			if(n == 1){
				continue;
			}
			cr8r_hash_insert(&is_prime, &htft_generic, &n, NULL);
		}
	}
	for(uint64_t i = 0; i < wheel_spokes; ++i){
		for(uint64_t j = 0; j < wheel_spokes; ++j){
			for(uint64_t n = wheel[i], m; n*n <= max; n += wheel_size){
				if(n == 1){
					continue;
				}
				m = wheel[j] + n - wheel[i];
				if(i > j){//ensure m >= n
					m += wheel_size;
				}
				for(uint64_t prod = n*m; prod <= max; prod += wheel_size*n){
					cr8r_hash_remove(&is_prime, &htft_generic, &prod);
				}
			}
		}
	}
	for(uint64_t i = 0; i < wheel_primes_count; ++i){
		if(!cr8r_vec_pushl(out, &cr8r_vecft_u64, wheel_primes + i)){
			cr8r_hash_destroy(&is_prime, &htft_generic);
			return false;
		}
	}
	for(uint64_t *prime = cr8r_hash_next(&is_prime, &htft_generic, NULL); prime; prime = cr8r_hash_next(&is_prime, &htft_generic, prime)){
		if(!cr8r_vec_pushr(out, &cr8r_vecft_u64, prime)){
			cr8r_hash_destroy(&is_prime, &htft_generic);
			return false;
		}
	}
	cr8r_hash_destroy(&is_prime, &htft_generic);
	cr8r_vec_trim(out, &cr8r_vecft_u64);
	cr8r_vec_sort(out, &cr8r_vecft_u64);
	return true;
}

int main(){
	fprintf(stderr, "\e[1;34mFinding numbers p**2 + q**3 + r**4 < 50000000 where p, q, r are prime...\e[0m\n");
	// p <= 7071, q <= 368, r <= 84
	cr8r_vec primes = {};
	if(!sieve_primes_silly(&primes, 7071)){
		fprintf(stderr, "\e[1;31mERROR: Could not initialize primes vector\e[0m\n");
		exit(1);
	}
	fprintf(stderr, "\e[1;32mFound %"PRIu64" primes\e[0m\n", cr8r_vec_len(&primes));
	uint64_t rb = 83, qb = 367, pb = 7069;
	rb = cr8r_vec_indexs(&primes, &cr8r_vecft_u64, &rb);
	qb = cr8r_vec_indexs(&primes, &cr8r_vecft_u64, &qb);
	pb = cr8r_vec_indexs(&primes, &cr8r_vecft_u64, &pb);
	cr8r_hashtbl_t ppts;
	if(!cr8r_hash_init(&ppts, &cr8r_htft_u64_void, (rb + 1)*(qb + 1)*(pb + 1)*2)){
		cr8r_vec_delete(&primes, &cr8r_vecft_u64);
		fprintf(stderr, "\e[1;31mERROR: Could not allocate hash table!\e[0m\n");
		exit(1);
	}
	cr8r_hashtbl_ft htft_replace;
	cr8r_hash_ft_init(&htft_replace, NULL, sizeof(uint64_t), cr8r_default_hash_u64, cr8r_default_cmp_u64, cr8r_default_replace, NULL);
	for(uint64_t k = 0; k <= rb; ++k){
		for(uint64_t j = 0; j <= qb; ++j){
			for(uint64_t i = 0; i <= pb; ++i){
				uint64_t p = *(uint64_t*)cr8r_vec_get(&primes, &cr8r_vecft_u64, i);
				uint64_t q = *(uint64_t*)cr8r_vec_get(&primes, &cr8r_vecft_u64, j);
				uint64_t r = *(uint64_t*)cr8r_vec_get(&primes, &cr8r_vecft_u64, k);
				uint64_t ppt = p*p + q*q*q + r*r*r*r;
				if(ppt >= 50000000){
					break;
				}
				cr8r_hash_append(&ppts, &htft_replace, &ppt, NULL);
			}
		}
	}
	cr8r_vec_delete(&primes, &cr8r_vecft_u64);
	fprintf(stderr, "\e[1;32mFound %"PRIu64" prime power triples!\e[0m\n", ppts.full);
	cr8r_hash_destroy(&ppts, &htft_replace);
}

