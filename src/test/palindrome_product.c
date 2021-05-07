// This test case is based on Project Euler problem 4
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <crater/hash.h>

int main(){
	fprintf(stderr, "\e[1;34mFinding largest palindrome product of 2 3-digit numbers...\e[0m\n");
	// We want 100001*a + 10010*b + 1100*c = A*B with 0 <= a, b, c <= 9 and 100 <= A, B <= 999
	// It would be nice to find a lower bound, so we try special forms.  100001*a is never the
	// product of 2 3-digit numbers 111111*k sometimes is, and we find
	// 888888 = 2*2*2*3*7*11*13*37 = 962*924.
	// So 888888 <= A*B <= 999999, 100 <= A, B <= 999 -> 888888/B <= A <= 999999/B ->
	// 888888/999 <= A <= 999999/B -> 890 <= A <= 999999/890 -> 890 <= A, B <= 999
	// So there will be at most (999 - 890 + 1)**2/2 candidate products, and we multiply by 2 and round
	// up a bit to find the size for the hash table
	// simply using a long bitmask instead of the hash table would be much faster, but not help test the library
	cr8r_hashtbl_t products;
	if(!cr8r_hash_init(&products, &cr8r_htft_u64_void, 16384)){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate hash table!\e[0m\n");
		exit(1);
	}
	for(uint64_t A = 890; A <= 999; ++A){
		for(uint64_t B = A; B <= 999; ++B){
			uint64_t product = A*B;
			int status;
			cr8r_hash_insert(&products, &cr8r_htft_u64_void, &product, &status);
		}
	}
	for(uint64_t a = 9; a >= 8; --a){
		for(uint64_t b = 10; b-- > 0;){
			for(uint64_t c = 10; c-- > 0;){
				uint64_t palindrome = 100001*a + 10010*b + 1100*c;
				if(cr8r_hash_get(&products, &cr8r_htft_u64_void, &palindrome)){
					cr8r_hash_destroy(&products, &cr8r_htft_u64_void);
					fprintf(stderr, "\e[1;32mLargest palindrome is %"PRIu64"\e[0m\n", palindrome);
					exit(0);
				}
			}
		}
	}
}

