#include "crater/container.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include <crater/vec.h>

static int test_pivot_mm(cr8r_vec *vec, cr8r_vec_ft *ft, cr8r_prng *prng){
	uint64_t tot_piv = 0, tot_piv2 = 0;
	for(uint64_t i = 0; i < 1000; ++i){
		cr8r_vec_shuffle(vec, ft, prng);
		uint64_t piv = *(uint64_t*)cr8r_vec_pivot_mm(vec, ft, 0, vec->len);
		if(piv < 299 || piv > 700){
			fprintf(stderr, "\e[1;31mInvalid pivot on trial %"PRIu64"/1000!\e[0m\n", i);
			return 0;
		}
		tot_piv += piv;
		tot_piv2 += piv*piv;
	}
	double avg_piv = tot_piv/1000.;
	double stdev_piv = sqrt(tot_piv2/1000. - avg_piv*avg_piv);
	fprintf(stderr, "Median by median of medians: %f +/- %f (expected 499.5)\n", avg_piv, stdev_piv);
	return 1;
}

static int test_partition(cr8r_vec *vec, cr8r_vec_ft *ft, cr8r_prng *prng){
	for(uint64_t i = 0; i < 1000; ++i){
		cr8r_vec_shuffle(vec, ft, prng);
		int64_t piv_idx = cr8r_vec_index(vec, ft, &i);
		uint64_t *piv = cr8r_vec_partition(vec, ft, 0, vec->len, cr8r_vec_get(vec, ft, piv_idx));
		if(*piv != i || (void*)piv != vec->buf + i*ft->base.size){
			fprintf(stderr, "\e[1;31mPartition placed pivot at wrong index on trial %"PRIu64"/1000!\e[0m\n", i);
			return 0;
		}
		for(uint64_t *it = vec->buf; it < piv; ++it){
			if(*it >= *piv){
				fprintf(stderr, "\e[1;31mPartition placed an element >= piv before piv on trial %"PRIu64"/1000!\e[0m\n", i);
				return 0;
			}
		}
		for(uint64_t *it = piv + 1; (void*)it < vec->buf + vec->len*ft->base.size; ++it){
			if(*it < *piv){
				fprintf(stderr, "\e[1;31mPartition place an element < piv after piv on trial %"PRIu64"/1000!\e[0m\n", i);
				return 0;
			}
		}
	}
	return 1;
}

static int test_ith(cr8r_vec *vec, cr8r_vec_ft *ft, cr8r_prng *prng){
	for(uint64_t i = 0; i < 1000; ++i){
		cr8r_vec_shuffle(vec, ft, prng);
		uint64_t *it = cr8r_vec_ith(vec, ft, 0, vec->len, i);
		if(*it != i){
			fprintf(stderr, "\e[1;31mFound the wrong ith element on trial %"PRIu64"/1000!\e[0m\n", i);
			return 0;
		}
	}
	return 1;
}

static int test_sort_end(cr8r_vec *vec, cr8r_vec_ft *ft, cr8r_prng *prng){
	for(uint64_t i = 0; i < 16; ++i){
		cr8r_vec_shuffle(vec, ft, prng);
		uint64_t *piv = cr8r_vec_ith(vec, ft, 0, vec->len, i);
		for(uint64_t *it = vec->buf; it < piv; ++it){
			if(*it > *piv){
				fprintf(stderr, "\e[1;31msort_i did not actually sort the first %"PRIu64"+1 elements\e[0m\n", i);
				return 0;
			}
		}
		for(uint64_t *it = piv + 1; (void*)it < vec->buf + vec->len*ft->base.size; ++it){
			if(*it < *piv){
				fprintf(stderr, "\e[1;31msort_i did not actually update the %"PRIu64"th element with later elements\e[0m\n", i);
				return 0;
			}
		}
	}
	return 1;
}

int main(){
	fprintf(stderr, "\e[1;34mTesting vector qselect support functions\e[0m\n");
	uint64_t tested = 0, passed = 0;
	cr8r_vec vec;
	cr8r_vec_ft ft = cr8r_vecft_u64;
	cr8r_vec_init(&vec, &ft, 1000);
	vec.len = vec.cap;
	for(uint64_t i = 0; i < vec.cap; ++i){
		*(uint64_t*)cr8r_vec_get(&vec, &ft, i) = i;
	}
	cr8r_prng *prng = cr8r_prng_init_lfg_m(0xc0d7dbfa9fcea4da);

	fprintf(stderr, "\e[1;34mTesting cr8r_vec_partition 1000x on a 1000 element array\e[0m\n");
	int status = test_partition(&vec, &ft, prng);
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_partition succeeded!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_partition failed!\e[0m\n");
	}

	fprintf(stderr, "\e[1;34mTesting cr8r_vec_sort_end\e[0m\n");
	status = test_sort_end(&vec, &ft, prng);
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_sort_end succeeded!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_sort_end failed!\e[0m\n");
	}

	fprintf(stderr, "\e[1;34mTesting cr8r_vec_pivot_mm 1000x on a 1000 element array\e[0m\n");
	status = test_pivot_mm(&vec, &ft, prng);
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_pivot_mm succeeded!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_pivot_mm failed!\e[0m\n");
	}

	fprintf(stderr, "\e[1;34mTesting cr8r_vec_ith 1000x on a 1000 element array\e[0m\n");
	status = test_ith(&vec, &ft, prng);
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_ith succeeded!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_ith failed!\e[0m\n");
	}

	free(prng);
	cr8r_vec_delete(&vec, &ft);
	if(passed == tested){
		fprintf(stderr, "\e[1;32mSuccess: passed %"PRIu64"/%"PRIu64" tests\e[0m\n", passed, tested);
	}else{
		fprintf(stderr, "\e[1;31mFailed: passed %"PRIu64"/%"PRIu64" tests\e[0m\n", passed, tested);
	}
}

