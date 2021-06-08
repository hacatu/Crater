#include "crater/vec.h"
#include <crater/kd_tree.h>

static inline int cmp_x_y(const cr8r_base_ft *base, const void *_a, const void *_b){
	const int64_t *points_a = _a, *points_b = _b ;
	if(points_a[0] < points_b[0]){
		return -1;
	}else if(points_a[0] == points_b[0]){
		return 0;
	}
	return 1;
}

static inline int cmp_z(const cr8r_base_ft *base, const void *_a, const void *_b){
	const int64_t *points_a = _a, *points_b = _b;
	if(points_a[2] < points_b[2]){
		return -1;
	}else if(points_a[2] == points_b[2]){
		return 0;
	}
	return 1;
}

static inline void kd_s2i64_ify_r(cr8r_vec *points, cr8r_vec_ft *ft, uint64_t a, uint64_t b){
	while(b > a){
		uint64_t mid_idx = (a + b)/2;
		int (*next_cmp)(const cr8r_base_ft*, const void*, const void*) = ft->cmp == cmp_z ? cmp_x_y : cmp_z;
		void *piv = cr8r_vec_ith(points, ft, a, b, mid_idx);
		piv = cr8r_vec_partition(points, ft, a, b, piv);
		ft->cmp = next_cmp;
		kd_s2i64_ify_r(points, ft, mid_idx + 1, b);
		ft->cmp = next_cmp;
		b = mid_idx;
	}
}

void cr8r_kd_s2i64_ify(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft){
	cr8r_vec_ft cft = *ft;
	cft.cmp = cmp_z;
	kd_s2i64_ify_r(&self->points, &cft, 0, self->points.len);
}

cr8r_kd_i64x3_pt *cr8r_kd_s2i64_nearest(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, const int64_t point[3]){
	cr8r_kd_i64x3_pt *mid = NULL;
	for(uint64_t a = 0, b = self->points.len, dir = 0; b > a; dir = 1 - dir){
		uint64_t mid_idx = (a + b)/2;// midpoint rounded up
		mid = cr8r_vec_get(&self->points, ft, mid_idx);
		int ord;
		if(dir == 0){
			ord = cmp_z(NULL, point, mid->coords);
		}else{
			ord = cmp_x_y(NULL, point, mid->coords);
		}
		if(ord < 0){
			b = mid_idx;
		}else if(ord >= 0){
			a = mid_idx;
		}// TODO: if ord == 0 we could check if *point == *mid
	}
	return mid;
}

static inline void kd_s2i64_forall_quad_r(cr8r_vec *points, cr8r_vec_ft *ft,
	const int64_t min_bound[3], const int64_t max_bound[3], uint64_t a, uint64_t b,
	void *data, void (*f)(void *data, const void *point)
){
	while(b > a){
		uint64_t mid_idx = (a + b)/2;
		int (*next_cmp)(const cr8r_base_ft*, const void*, const void*) = ft->cmp == cmp_z ? cmp_x_y : cmp_z;
		void *piv = cr8r_vec_get(points, ft, mid_idx);
		bool visit_lt_part = ft->cmp(&ft->base, piv, min_bound) >= 0;
		bool visit_ge_part = ft->cmp(&ft->base, piv, max_bound) <= 0;
		if(visit_lt_part && visit_ge_part){
			f(data, piv);
			ft->cmp = next_cmp;
			kd_s2i64_forall_quad_r(points, ft, min_bound, max_bound, mid_idx + 1, b, data, f);
		}
		ft->cmp = next_cmp;
		if(visit_lt_part){
			b = mid_idx;
		}else if(visit_ge_part){
			a = mid_idx + 1;
		}else{
			return;
		}
	}
}

void cr8r_kd_s2i64_forall_quad(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, const int64_t min_bound[3], const int64_t max_bound[3], void *data, void (*f)(void *data, const void *point)){
	cr8r_vec_ft cft = *ft;
	cft.cmp = cmp_z;
	kd_s2i64_forall_quad_r(&self->points, &cft, min_bound, max_bound, 0, self->points.len, data, f);
}

void kd_s2i64_forall_ball_r(cr8r_vec *points, cr8r_vec_ft *ft, const int64_t point[3], int64_t sqdist, uint64_t a, uint64_t b, void *data, void (*f)(void *data, const void *point)){
	
}

void cr8r_kd_s2i64_forall_ball(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, const int64_t point[3], int64_t sqdist, void *data, void (*f)(void *data, const void *point)){
	cr8r_vec_ft cft = *ft;
	cft.cmp = cmp_z;
	kd_s2i64_forall_ball_r(&self->points, &cft, point, sqdist, 0, self->points.len, data, f);
}

