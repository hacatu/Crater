#include "crater/kd_check.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include <crater/kd_tree.h>

int main(){
	uint64_t tested = 0, passed = 0;
	cr8r_prng *prng = cr8r_prng_init_lcg(0x555b6745db2f2b85);
	cr8r_vec points = {};
	cr8r_vec_init(&points, &cr8r_kdft_c3i64.super, 1000);
	fprintf(stderr, "\e[1;34mGenerating 1000 random lattice points within [-1000, 1000]^3\e[0m\n");
	for(uint64_t i = 0; i < points.cap; ++i){
		int64_t point[3];
		for(uint64_t j = 0; j < 3; ++j){
			int64_t x = cr8r_prng_uniform_u64(prng, 0, 2000);
			point[j] = x - 1000;
		}
		cr8r_vec_pushr(&points, &cr8r_kdft_c3i64.super, point);
	}
	fprintf(stderr, "\e[1;34mChecking bounds of points\e[0m\n");
	cr8r_kdwin_s2i64 bounds = {}, bounds1 = {};
	cr8r_kdwin_bounding_i64x3(&bounds, &points, &cr8r_kdft_c3i64);
	memcpy(&bounds1.bl, points.buf, cr8r_kdft_c3i64.super.base.size);
	memcpy(&bounds1.tr, points.buf, cr8r_kdft_c3i64.super.base.size);
	for(uint64_t i = 1; i < points.len; ++i){
		const int64_t *point = cr8r_vec_get(&points, &cr8r_kdft_c3i64.super, i);
		for(uint64_t j = 0; j < 3; ++j){
			if(point[j] < bounds1.bl[j]){
				bounds1.bl[j] = point[j];
			}else if(point[j] > bounds1.tr[j]){
				bounds1.tr[j] = point[j];
			}
		}
	}
	++tested;
	if(memcmp(&bounds, &bounds1, sizeof(cr8r_kdwin_s2i64))){
		fprintf(stderr, "\e[1;31mcr8r_kdwin_bounding_i64x3 didn't compute the correct bounds!\e[0m\n");
	}else{
		fprintf(stderr, "\e[1;32mFound bounds (%"PRIi64",%"PRIi64",%"PRIi64"):(%"PRIi64",%"PRIi64",%"PRIi64")\e[0m\n", bounds.bl[0], bounds.bl[1], bounds.bl[2], bounds.tr[0], bounds.tr[1], bounds.tr[2]);
		++passed;
	}

	int status = cr8r_kd_ify(&points, &cr8r_kdft_c3i64, 0, points.len);
	++tested;
	if(status){
		if(cr8r_kd_check_tree(&points, &cr8r_kdft_c3i64, 0, points.len)){
			fprintf(stderr, "\e[1;32mKD Tree built successfully!\e[0m\n");
			++passed;
		}else{
			fprintf(stderr, "\e[1;31mKD Tree built wrong!\e[0m\n");
		}
	}else{
		fprintf(stderr, "\e[1;32mKD Tree build failed!\e[0m\n");
	}

	cr8r_vec_delete(&points, &cr8r_kdft_c3i64.super);
	free(prng);
}

