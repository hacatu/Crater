#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include <crater/vec.h>

static int test_all_bsearches(int64_t n, cr8r_vec *vec){
	/* We will create sorted lists of n integers with every possible way for the integers to repeat.
	 * That is, all the integers could be distinct, all could be equal, the first three could be equal and the rest distinct, and so on.
	 * Since a sorted list of integers satisfies a_1 <= a_2 <= ... <= a_n, and since we don't care how big the difference between distinct numbers is,
	 * we can simply choose each of the n - 1 "<=" inequalites to either be a difference of 0 or 1.
	 * In the following code, "mask" is this choice.  So we set the first number to 1, and then look at the bits of mask one by one when setting the
	 * subsequent numbers.  If a bit is 1, the next number is 1 greater than the previous, otherwise it is the same.
	 */
	uint64_t max_mask = (1ull << (n - 1)) - 1;
	uint64_t *buf = vec->buf;
	vec->len = n;
	uint64_t num_tests = 0;
	uint64_t gts_failed = 0;
	uint64_t ges_failed = 0;
	uint64_t lts_failed = 0;
	uint64_t les_failed = 0;
	for(uint64_t mask = 0; mask <= max_mask; ++mask){
		buf[0] = 1;
		for(int64_t i = 1, d = 1; i < n; ++i){
			if(mask&(1ull << (i - 1))){
				++d;
			}
			buf[i] = d;
		}
		// Include 0 and buf[n - 1] + 1 to also test searching for keys outside the range
		for(uint64_t e = 0; e <= buf[n - 1] + 1; ++e){
			int64_t idx;
			for(idx = 0; idx < n && buf[idx] <= e; ++idx);
			if(idx == n){
				idx = -1;
			}
			if(idx != cr8r_vec_first_gts(vec, &cr8r_vecft_u64, &e)){
				if(!gts_failed++){
					fprintf(stderr, "\e[1;31mcr8r_vec_first_gts(mask=%"PRIu64") failed!\e[0m\n", mask);
				}
			}
			for(idx = 0; idx < n && buf[idx] < e; ++idx);
			if(idx == n){
				idx = -1;
			}
			if(idx != cr8r_vec_first_ges(vec, &cr8r_vecft_u64, &e)){
				if(!ges_failed++){
					fprintf(stderr, "\e[1;31mcr8r_vec_first_ges(mask=%"PRIu64") failed!\e[0m\n", mask);
				}
			}
			for(idx = n - 1; idx >= 0 && buf[idx] >= e; --idx);
			if(idx != cr8r_vec_last_lts(vec, &cr8r_vecft_u64, &e)){
				if(!lts_failed++){
					fprintf(stderr, "\e[1;31mcr8r_vec_last_lts(mask=%"PRIu64") failed!\e[0m\n", mask);
				}
			}
			for(idx = n - 1; idx >= 0 && buf[idx] > e; --idx);
			if(idx != cr8r_vec_last_les(vec, &cr8r_vecft_u64, &e)){
				if(!les_failed++){
					fprintf(stderr, "\e[1;31mcr8r_vec_last_les(mask=%"PRIu64") failed!\e[0m\n", mask);
				}
			}
		}
		++num_tests;
	}
	fprintf(stderr, "%s: %"PRIu64"/%"PRIu64" cr8r_vec_first_gts tests passed\e[0m\n", gts_failed ? "\e[1;31mFailed" : "\e[1;32mSuccess", (num_tests - gts_failed), num_tests);
	fprintf(stderr, "%s: %"PRIu64"/%"PRIu64" cr8r_vec_first_ges tests passed\e[0m\n", ges_failed ? "\e[1;31mFailed" : "\e[1;32mSuccess", (num_tests - ges_failed), num_tests);
	fprintf(stderr, "%s: %"PRIu64"/%"PRIu64" cr8r_vec_last_lts tests passed\e[0m\n", lts_failed ? "\e[1;31mFailed" : "\e[1;32mSuccess", (num_tests - lts_failed), num_tests);
	fprintf(stderr, "%s: %"PRIu64"/%"PRIu64" cr8r_vec_last_les tests passed\e[0m\n", les_failed ? "\e[1;31mFailed" : "\e[1;32mSuccess", (num_tests - les_failed), num_tests);
	return !(gts_failed | ges_failed | lts_failed | les_failed);
}

int main(){
	fprintf(stderr, "\e[1;34mTesting vector qselect support functions\e[0m\n");
	uint64_t _dummy;
	cr8r_vec vec;
	cr8r_vec_init(&vec, &cr8r_vecft_u64, 10);
	int status = -1 == cr8r_vec_first_gts(&vec, &cr8r_vecft_u64, &_dummy);
	fprintf(stderr, status ? "\e[1;32mSuccess: cr8r_vec_first_gts([]) test passed\e[0m\n" : "\e[1;31mFailed: cr8r_vec_first_gts([]) test failed\e[0m\n");
	status = -1 == cr8r_vec_first_ges(&vec, &cr8r_vecft_u64, &_dummy);
	fprintf(stderr, status ? "\e[1;32mSuccess: cr8r_vec_first_ges([]) test passed\e[0m\n" : "\e[1;31mFailed: cr8r_vec_first_ges([]) test failed\e[0m\n");
	status = -1 == cr8r_vec_last_lts(&vec, &cr8r_vecft_u64, &_dummy);
	fprintf(stderr, status ? "\e[1;32mSuccess: cr8r_vec_last_lts([]) test passed\e[0m\n" : "\e[1;31mFailed: cr8r_vec_last_lts([]) test failed\e[0m\n");
	status = -1 == cr8r_vec_last_les(&vec, &cr8r_vecft_u64, &_dummy);
	fprintf(stderr, status ? "\e[1;32mSuccess: cr8r_vec_last_les([]) test passed\e[0m\n" : "\e[1;31mFailed: cr8r_vec_last_les([]) test failed\e[0m\n");
	for(uint64_t n = 1; n <= vec.cap; ++n){
		fprintf(stderr, "\e[1;34mn=%"PRIu64"\e[0m\n", n);
		test_all_bsearches(n, &vec);
	}
	cr8r_vec_delete(&vec, &cr8r_vecft_u64);
}

