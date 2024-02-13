#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include <crater/kd_check.h>
#include <crater/kd_tree.h>

static cr8r_kd_k_closest_state point_kcs;

#define NUM_POINTS 1000
#define BOX_SIZE 2000
#define KCS_SIZE 2200
#define KCS_COUNT 50
#define KCS_TRIALS 50
#define KD_TRIALS 5

static void print_kcs(const cr8r_vec *points){
	for(uint64_t i = 0; i < points->len; ++i){
		int64_t *point = points->buf + i*point_kcs.ft.super.base.size;
		double dist = point_kcs.ft.sqdist(&point_kcs.ft, point_kcs.pt, point);
		fprintf(stderr, "%.0f: (%"PRIi64", %"PRIi64", %"PRIi64"), ", dist, point[0], point[1], point[2]);
	}
	fprintf(stderr, "\n");
}

static void get_bounds(cr8r_vec *points, cr8r_kdwin_s2i64 *bounds){
	memcpy(bounds->bl, points->buf, cr8r_kdft_c3i64.super.base.size);
	memcpy(bounds->tr, points->buf, cr8r_kdft_c3i64.super.base.size);
	for(uint64_t i = 1; i < points->len; ++i){
		const int64_t *point = cr8r_vec_get(points, &cr8r_kdft_c3i64.super, i);
		for(uint64_t j = 0; j < 3; ++j){
			if(point[j] < bounds->bl[j]){
				bounds->bl[j] = point[j];
			}else if(point[j] > bounds->tr[j]){
				bounds->tr[j] = point[j];
			}
		}
	}
}

int main(){
	point_kcs.ft = cr8r_kdft_c3i64;
	point_kcs.ft.super.cmp = cr8r_default_cmp_kd_kcs_pt_dist;
	uint64_t tested = 0, passed = 0;
	cr8r_prng *prng = cr8r_prng_init_lcg(0x555b6745db2f2b85);
	cr8r_vec points = {}, res_points1 = {}, res_points2;
	cr8r_vec_init(&points, &cr8r_kdft_c3i64.super, NUM_POINTS);
	cr8r_vec_init(&res_points1, &point_kcs.ft.super, KCS_COUNT + 1);
	cr8r_vec_init(&res_points2, &point_kcs.ft.super, KCS_COUNT + 1);
	for(uint64_t trial = 0; trial < KD_TRIALS; ++trial){
		cr8r_vec_clear(&points, &cr8r_kdft_c3i64.super);
		fprintf(stderr, "\e[1;34mGenerating %1$d random lattice points within [-%2$d, %2$d]^3\e[0m\n", NUM_POINTS, BOX_SIZE/2);
		for(uint64_t i = 0; i < points.cap; ++i){
			int64_t point[3];
			for(uint64_t j = 0; j < 3; ++j){
				int64_t x = cr8r_prng_uniform_u64(prng, 0, BOX_SIZE + 1);
				point[j] = x - BOX_SIZE/2;
			}
			cr8r_vec_pushr(&points, &cr8r_kdft_c3i64.super, point);
		}
		fprintf(stderr, "\e[1;34mChecking bounds of points\e[0m\n");
		cr8r_kdwin_s2i64 bounds = {}, bounds1 = {};
		cr8r_kdwin_bounding_i64x3(&bounds, &points, &cr8r_kdft_c3i64);
		get_bounds(&points, &bounds1);
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
			fprintf(stderr, "\e[1;31mKD Tree build failed!\e[0m\n");
		}

		fprintf(stderr, "\e[1;34mFinding %1$d closest points for %2$d random points in [-%3$d, %3$d]^3 ...\e[0m\n", KCS_COUNT, KCS_TRIALS, KCS_SIZE);
		for(uint64_t i = 0; i < KCS_TRIALS; ++i){
			++tested;
			int64_t point[3];
			for(uint64_t j = 0; j < 3; ++j){
				int64_t x = cr8r_prng_uniform_u64(prng, 0, KCS_SIZE + 1);
				point[j] = x - KCS_SIZE/2;
			}
			fprintf(stderr, "\e[1;34m - (%"PRIi64", %"PRIi64", %"PRIi64")\e[0m\n", point[0], point[1], point[2]);
			cr8r_kd_k_closest(&points, &cr8r_kdft_c3i64, &bounds, point, KCS_COUNT, &res_points1);
			if(res_points1.len != KCS_COUNT){
				fprintf(stderr, "\e[1;31mkd_k_closest found %"PRIu64" points (%d requested)\e[0m\n", res_points1.len, KCS_COUNT);
				continue;
			}
			cr8r_kd_k_closest_naive(&points, &cr8r_kdft_c3i64, &bounds, point, KCS_COUNT, &res_points2);
			if(res_points2.len != KCS_COUNT){
				fprintf(stderr, "\e[1;31mkd_k_closest_naive found %"PRIu64" points (%d requested)\e[0m\n", res_points2.len, KCS_COUNT);
				cr8r_vec_clear(&res_points2, &point_kcs.ft.super);
				continue;
			}
			point_kcs.pt = point;
			cr8r_vec_sort(&res_points1, &point_kcs.ft.super);
			cr8r_vec_sort(&res_points2, &point_kcs.ft.super);
			bool is_same = true;
			for(uint64_t j = 0; j < KCS_COUNT; ++j){
				double d_a = point_kcs.ft.sqdist(&point_kcs.ft, point_kcs.pt, cr8r_vec_get(&res_points1, &point_kcs.ft.super, j));
				double d_b = point_kcs.ft.sqdist(&point_kcs.ft, point_kcs.pt, cr8r_vec_get(&res_points2, &point_kcs.ft.super, j));
				if(d_a != d_b){
					if(is_same){
						print_kcs(&res_points1);
						print_kcs(&res_points2);
					}
					is_same = false;
				}
			}
			if(is_same){
				++passed;
			}else{
				fprintf(stderr, "\e[1;31mkd_k_closest did not produce the same points as naive search!\e[0m\n");
			}
		}
	}

	if(passed == tested){
		fprintf(stderr, "\e[1;32mSuccess: passed %"PRIu64"/%"PRIu64" tests\e[0m\n", passed, tested);
	}else{
		fprintf(stderr, "\e[1;31mFailed: passed %"PRIu64"/%"PRIu64" tests\e[0m\n", passed, tested);
	}
	cr8r_vec_delete(&points, &cr8r_kdft_c3i64.super);
	cr8r_vec_delete(&res_points1, &point_kcs.ft.super);
	cr8r_vec_delete(&res_points2, &point_kcs.ft.super);
	free(prng);
}

