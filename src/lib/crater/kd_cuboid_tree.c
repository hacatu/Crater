#include "crater/vec.h"
#include <crater/kd_tree.h>
#include <string.h>

static inline int cmp_xi(const cr8r_base_ft *base, const void *_a, const void *_b){
	uint64_t i = (uint64_t)base->data;
	const int64_t *points_a = _a, *points_b = _b;
	if(points_a[i] < points_b[i]){
		return -1;
	}else if(points_a[i] == points_b[i]){
		return 0;
	}
	return 1;
}

static inline void kd_c3i64_ify_r(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b){
	while(b > a){
		uint64_t mid_idx = (a + b)/2;
		uint64_t next_i = ((uint64_t)ft->base.data + 1)%3;
		cr8r_kd_i64x3_pt *piv = cr8r_vec_ith(&self->points, ft, a, b, mid_idx);
		piv = cr8r_vec_partition(&self->points, ft, a, b, piv);
		for(uint64_t i = 0; i < 3; ++i){
			if(self->bounds.a[i] > piv->coords[i]){
				self->bounds.a[i] = piv->coords[i];
			}
			if(self->bounds.b[i] < piv->coords[i]){
				self->bounds.b[i] = piv->coords[i];
			}
		}
		ft->base.data = (void*)next_i;
		kd_c3i64_ify_r(self, ft, mid_idx + 1, b);
		ft->base.data = (void*)next_i;
		b = mid_idx;
	}
}

void cr8r_kd_c3i64_ify(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft){
	cr8r_vec_ft cft = *ft;
	cft.cmp = cmp_xi;
	cft.base.data = (void*)0;
	if(self->points.len){
		memcpy(self->bounds.a, self->points.buf, 3*sizeof(int64_t));
		memcpy(self->bounds.b, self->points.buf, 3*sizeof(int64_t));
	}
	kd_c3i64_ify_r(self, &cft, 0, self->points.len);
}

cr8r_kd_i64x3_pt *cr8r_kd_c3i64_nearest(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft, const int64_t point[3]){
	cr8r_kd_i64x3_pt *mid = NULL;
	ft->base.data = (void*)0;
	for(uint64_t a = 0, b = self->points.len; b > a;){
		uint64_t mid_idx = (a + b)/2;// midpoint rounded up
		mid = cr8r_vec_get(&self->points, ft, mid_idx);
		int ord = cmp_xi(&ft->base, point, mid->coords);
		if(ord < 0){
			b = mid_idx;
		}else if(ord >= 0){
			a = mid_idx;
		}// TODO: if ord == 0 we could check if *point == *mid
		ft->base.data = (void*)(((uint64_t)ft->base.data + 1)%3);
	}
	return mid;
}

int cr8r_kd_c3i64_intersect_quad_quad(cr8r_kd_c3i64_quad query, cr8r_kd_c3i64_quad window){
	cr8r_kd_c3i64_quad res_quad = window;
	int res = 2;
	for(uint64_t i = 0; i < 3; ++i){
		if(query.a[i] > res_quad.a[i]){
			res_quad.a[i] = query.a[i];
			res = !!res;
		}
		if(query.b[i] < res_quad.b[i]){
			res_quad.b[i] = query.b[i];
			res = !!res;
		}
		if(res == 1 && res_quad.b[i] < res_quad.a[i]){
			res = 0;
		}
	}
	return res;
}

static inline bool c3i64_quad_contains(cr8r_kd_c3i64_quad query, int64_t point[3]){
	for(uint64_t i = 0; i < 3; ++i){
		if(point[i] < query.a[i] || query.b[i] < point[i]){
			return 0;
		}
	}
	return 1;
}

static inline void kd_c3i64_forall_r(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b, void *data, void (*f)(void *data, const void *point)){
	if(a < b){
		uint64_t mid_idx = (a + b)/2;
		cr8r_kd_i64x3_pt *piv = cr8r_vec_get(&self->points, ft, mid_idx);
		f(data, piv);
		kd_c3i64_forall_r(self, ft, a, mid_idx, data, f);
		kd_c3i64_forall_r(self, ft, mid_idx + 1, b, data, f);
	}
}

static inline void kd_c3i64_forall_quad_r(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft,
	cr8r_kd_c3i64_quad query, cr8r_kd_c3i64_quad window, uint64_t a, uint64_t b,
	void *data, void (*f)(void *data, const void *point)
){
	if(b > a){
		uint64_t mid_idx = (a + b)/2;
		uint64_t cur_i = (uint64_t)ft->base.data;
		cr8r_kd_i64x3_pt *piv = cr8r_vec_get(&self->points, ft, mid_idx);cr8r_kd_c3i64_quad first_subwindow = window;
		cr8r_kd_c3i64_quad second_subwindow = window;
		first_subwindow.b[cur_i] = piv->coords[cur_i];
		second_subwindow.a[cur_i] = piv->coords[cur_i];
		if(c3i64_quad_contains(query, piv->coords)){
			f(data, piv);
		}
		int first_status = cr8r_kd_c3i64_intersect_quad_quad(query, first_subwindow);
		int second_status = cr8r_kd_c3i64_intersect_quad_quad(query, second_subwindow);
		ft->base.data = (void*)((cur_i + 1)%3);
		if(first_status == 1){
			kd_c3i64_forall_quad_r(self, ft, query, first_subwindow, a, mid_idx, data, f);
		}else if(first_status == 2){
			kd_c3i64_forall_r(self, ft, a, mid_idx, data, f);
		}
		ft->base.data = (void*)((cur_i + 1)%3);
		if(second_status == 1){
			kd_c3i64_forall_quad_r(self, ft, query, second_subwindow, mid_idx + 1, b, data, f);
		}else if(first_status == 2){
			kd_c3i64_forall_r(self, ft, mid_idx + 1, b, data, f);
		}
	}
}

void cr8r_kd_c3i64_forall_quad(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft, cr8r_kd_c3i64_quad query, void *data, void (*f)(void *data, const void *point)){
	cr8r_vec_ft cft = *ft;
	cft.base.data = (void*)data;
	kd_c3i64_forall_quad_r(self, &cft, query, self->bounds, 0, self->points.len, data, f);
}

int cr8r_kd_c3i64_intersect_ball_quad(cr8r_kd_c3i64_ball query, cr8r_kd_c3i64_quad window){
	return 1;//TODO
}

static inline void kd_c3i64_forall_ball_r(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft,
	cr8r_kd_c3i64_ball query, cr8r_kd_c3i64_quad window, uint64_t a, uint64_t b,
	void *data, void (*f)(void *data, const void *point)
){
	if(a < b){
		uint64_t mid_idx = (a + b)/2;
		uint64_t cur_i = (uint64_t)ft->base.data;
		cr8r_kd_i64x3_pt *piv = cr8r_vec_get(&self->points, ft, mid_idx);
		cr8r_kd_c3i64_quad first_subwindow = window;
		cr8r_kd_c3i64_quad second_subwindow = window;
		first_subwindow.b[cur_i] = piv->coords[cur_i];
		second_subwindow.a[cur_i] = piv->coords[cur_i];
		if(cr8r_kd_i64x3_sqdist(query.p, piv->coords) <= query.sqdist){
			f(data, piv);
		}
		int first_status = cr8r_kd_c3i64_intersect_ball_quad(query, first_subwindow);
		int second_status = cr8r_kd_c3i64_intersect_ball_quad(query, second_subwindow);
		ft->base.data = (void*)((cur_i + 1)%3);
		if(first_status == 1){
			kd_c3i64_forall_ball_r(self, ft, query, first_subwindow, a, mid_idx, data, f);
		}else if(first_status == 2){
			kd_c3i64_forall_r(self, ft, a, mid_idx, data, f);
		}
		ft->base.data = (void*)((cur_i + 1)%3);
		if(second_status == 1){
			kd_c3i64_forall_ball_r(self, ft, query, second_subwindow, mid_idx + 1, b, data, f);
		}else if(second_status == 2){
			kd_c3i64_forall_r(self, ft, mid_idx + 1, b, data, f);
		}
	}
}

void cr8r_kd_c3i64_forall_ball(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft, cr8r_kd_c3i64_ball query, void *data, void (*f)(void *data, const void *point)){
	cr8r_vec_ft cft = *ft;
	cft.base.data = (void*)0;
	cr8r_kd_c3i64_quad window = self->bounds;
	kd_c3i64_forall_ball_r(self, &cft, query, window, 0, self->points.len, data, f);
}

void cr8r_kd_c3i64_forall(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft, void *data, void (*f)(void *data, const void *point)){
	cr8r_vec_ft cft = *ft;
	cft.base.data = (void*)0;
	kd_c3i64_forall_r(self, &cft, 0, self->points.len, data, f);
}

