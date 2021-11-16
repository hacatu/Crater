#include "crater/container.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <crater/vec.h>

static const uint8_t max_counts[] = {[1]=2, [2]=2, [7]=2, [8]=1, [10]=7};
//static const uint8_t max_counts[] = {[4]=1, [5]=2, [8]=1, [10]=4};
//static const uint8_t max_counts[] = {[1]=1, [4]=1, [5]=1, [10]=3};
static const uint64_t facts[] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880};

static void nontrivial_copy(cr8r_base_ft *ft, void *dest, const void *src){
	memcpy(dest, src, ft->size);
}

static cr8r_vec_ft vecft_u8x10 = {
	.base.size = 10*sizeof(uint8_t),
	.new_size = cr8r_default_new_size,
	.resize = cr8r_default_resize,
	.copy = nontrivial_copy,
	.swap = cr8r_default_swap
};

static uint64_t matching_dfs_perm(const uint8_t digit_counts[static 10]){
	uint8_t dfs_counts[10] = {};
	uint64_t dfs = 0;
	for(uint64_t digit = 1; digit < 10; ++digit){
		dfs += digit_counts[digit]*facts[digit];
	}
	uint64_t res = dfs;
	for(uint64_t zeros = 0; zeros <= dfs_counts[0]; ++zeros){
		dfs = res + zeros;
		memset(dfs_counts, 0, sizeof(dfs_counts));
		while(dfs){
			dfs_counts[dfs%10]++;
			dfs /= 10;
		}
		if(dfs_counts[0] <= digit_counts[0] && !memcmp(dfs_counts + 1, digit_counts + 1, 9*sizeof(uint8_t))){
			return res + zeros;
		}
	}
	return 0;
}

static bool has_dfs_perm(const cr8r_vec_ft *ft, const void *ent, void *data){
	return !!matching_dfs_perm(ent);
}

static bool generate_candidates(cr8r_vec *out, bool top, uint64_t digit, uint8_t head_counts[static 11], const uint8_t max_counts[static 11]){
	bool status = true;
	uint64_t head_count = head_counts[10];
	if(head_count == max_counts[10]){
		status = cr8r_vec_pushr(out, &vecft_u8x10, head_counts);
	}else if(top){
		head_counts[digit] = max_counts[digit];
		head_counts[10] = head_count + max_counts[digit];
		status = status && generate_candidates(out, true, digit + 1, head_counts, max_counts);
		for(uint64_t i = max_counts[digit] + 1; status && head_count + i <= max_counts[10]; ++i){
			head_counts[digit] = i;
			head_counts[10] = head_count + i;
			status = status && generate_candidates(out, false, digit + 1, head_counts, max_counts);
		}
	}else if(digit < 9){
		for(uint64_t i = 0; status && head_count + i <= max_counts[10]; ++i){
			head_counts[digit] = i;
			head_counts[10] = head_count + i;
			status = status && generate_candidates(out, false, digit + 1, head_counts, max_counts);
		}
	}else{
		head_counts[9] = max_counts[10] - head_count;
		head_counts[10] = max_counts[10];
		status = cr8r_vec_pushr(out, &vecft_u8x10, head_counts);
	}
	head_counts[digit] = 0;
	return status;
}

int main(){
	fprintf(stderr, "\e[1;34mSearching for numbers equal to their own digit factorial sums...\e[0m\n");
	cr8r_vec candidates = {};
	uint8_t head_counts[11] = {};
	if(!generate_candidates(&candidates, true, 0, head_counts, max_counts)){
		fprintf(stderr, "\e[1;31mERROR: generate_candidates failed (oom)!\e[0m\n");
		exit(EXIT_FAILURE);
	}
	cr8r_vec solutions = {};
	if(!cr8r_vec_filtered(&solutions, &candidates, &vecft_u8x10, has_dfs_perm, NULL)){
		cr8r_vec_delete(&candidates, &vecft_u8x10);
		fprintf(stderr, "\e[1;31mERROR: cr8r_vec_filtered failed (oom)!\e[0m\n");
		exit(EXIT_FAILURE);
	}
	cr8r_vec_delete(&candidates, &vecft_u8x10);
	fprintf(stderr, "\e[1;33mFound %"PRIu64" solutions: \e[0m", solutions.len);
	uint64_t s = 0;
	for(uint64_t i = 0; i < solutions.len; ++i){
		uint64_t x = matching_dfs_perm(cr8r_vec_get(&solutions, &vecft_u8x10, i));
		fprintf(stderr, "\e[1;33m%"PRIu64", \e[0m", x);
		s += x;
	}
	cr8r_vec_delete(&solutions, &vecft_u8x10);
	s -= 3;
	if(s == 40730){
		fprintf(stderr, "\n\e[1;32mSum of solutions is %"PRIu64"\nSuccess: passed 1/1 tests!\e[0m\n", s);
	}else{
		fprintf(stderr, "\n\e[1;31mGot incorrect sum of solutions (%"PRIu64")\nFailed: pased 0/1 tests!\e[0m\n", s);
	}
}

