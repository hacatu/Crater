#include "crater/container.h"
#include "crater/vec.h"
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>

#include <crater/kd_tree.h>
#include <crater/minmax_heap.h>

static int cmp_depth_i64cu(const cr8r_base_ft *_ft, const void *_a, const void *_b){
	const cr8r_kd_ft *ft = (cr8r_kd_ft*)_ft;
	const int64_t *a = _a;
	const int64_t *b = _b;
	uint64_t depth = (uint64_t)ft->super.base.data;
	uint64_t idx = depth%ft->dim;
	int64_t key_a = a[idx];
	int64_t key_b = b[idx];
	if(key_a < key_b){
		return -1;
	}else if(key_a > key_b){
		return 1;
	}
	return 0;
}

static int cmp_depth_i64sp(const cr8r_base_ft *_ft, const void *_a, const void *_b){
	const cr8r_kd_ft *ft = (cr8r_kd_ft*)_ft;
	const int64_t *a = _a;
	const int64_t *b = _b;
	uint64_t depth = (uint64_t)ft->super.base.data;
	uint64_t idx = depth%ft->dim;
	if(idx == ft->dim - 1){
		int64_t a_x = a[0], a_y = a[1];
		int64_t b_x = b[0], b_y = b[1];
		// if a and b are in opposite half planes
		if((a_y >= 0) != (b_y >= 0)){
			if(a_y < 0){
				#ifdef CR8R_KDSP_ORIGIN_IS_INFIMUM
					return 1;
				#else // otherwise (0, 0) == any point
					return b_x || b_y;// return 0 (==) if b is (0, 0), else 1 (>)
				#endif
			}
			#ifdef CR8R_KDSP_ORIGIN_IS_INFIMUM
				return -1;
			#else // otherwise (0, 0) == any point
				return -(a_x || a_y);// return 0 (==) if a is (0, 0), else -1 (<)
			#endif
		}
		int64_t crossp = a_x*b_y - b_x*a_y;
		if(crossp > 0){
			return -1;
		}else if(crossp < 0){
			return 1;
		}
		#ifdef CR8R_KDSP_ORIGIN_IS_INFIMUM
			if(!a_x && !a_y){// if a is (0, 0)
				return -(b_x || b_y);// return 0 (==) if b is (0, 0), else -1 (<)
			}
			return !(b_x || b_y);// return 1 (>) if b is (0, 0), else 0 (==)
		#else
			return 0;
		#endif
	}
	int64_t key_a = a[2 + idx];
	int64_t key_b = b[2 + idx];
	if(key_a < key_b){
		return -1;
	}else if(key_a > key_b){
		return 1;
	}
	return 0;
}

static void split_i64sp(const cr8r_kd_ft *ft, const void *_self, const void *_root_pt, void *_o1, void *_o2){
	const cr8r_kdwin_s2i64 *self = _self;
	const int64_t *root = _root_pt;
	cr8r_kdwin_s2i64 *o1 = _o1;
	cr8r_kdwin_s2i64 *o2 = _o2;
	uint64_t idx = (uint64_t)ft->super.base.data%ft->dim;
	memcpy(o1, self, sizeof(cr8r_kdwin_s2i64));
	memcpy(o2, self, sizeof(cr8r_kdwin_s2i64));
	if(idx == ft->dim - 1){
		if(!root[0] || !root[1]){
			memcpy(o1->tr, root, 2*sizeof(int64_t));
			memcpy(o2->bl, root, 2*sizeof(int64_t));
		}
	}else{
		o1->tr[2 + idx] = root[2 + idx];
		o2->bl[2 + idx] = root[2 + idx];
	}
}

static void update_i64sp(const cr8r_kd_ft *_ft, void *_self, const void *_pt){
	cr8r_kdwin_s2i64 *self = _self;
	const int64_t *pt = _pt;
	cr8r_kd_ft ft = *_ft;
	for(uint64_t i = 0; i < ft.dim - 1; ++i){
		ft.super.base.data = (void*)i;
		if(ft.super.cmp(&ft.super.base, pt, self->bl) < 0){
			self->bl[2 + i] = pt[2 + i];
		}else if(ft.super.cmp(&ft.super.base, pt, self->tr) > 0){
			self->tr[2 + i] = pt[2 + i];
		}
	}
	ft.super.base.data = (void*)(ft.dim - 1);
	if(ft.super.cmp(&ft.super.base, pt, self->bl) < 0){
		memcpy(self->bl, pt, 2*sizeof(int64_t));
	}else if(ft.super.cmp(&ft.super.base, pt, self->tr) > 0){
		memcpy(self->tr, pt, 2*sizeof(int64_t));
	}
}

static double min_sqdist_i64sp(const cr8r_kd_ft *_ft, const void *_self, const void *_pt){
	/* The min sqdist on a sphere is the same in the last n-2 dimensions,
	but the first 2 dimensions which are treated as an angle must have their
	sqdist computed differently
	The distance beween x1, x2 and l1, l2 can be found by the law of cosines
	sqdist = 2*x1**2 + 2*x2**2 - 2*(x1**2 + x2**2)*cos
	Here, x1**2 + x2**2 is the square of the radius of the circle in the hypersphere
	where x1, x2 lives.  The triangle in question in the law of cosines
	is in the circle with x1, x2 and is an isosolese triangle through the center
	and two points on the circumference
	Thus the two known sides are the radius of that circle and the cos can be
	found through the dot product
	sqdist = 2*(x1**2 + x2**2 - (x1*l1 + x2*l2)/(l1**2 + l2**2))
	Remember, this is just the component of the sqdist in the first two dimensions
	Also notice that this is not necessarily an integer, so we will use floor division
	and accept that we explore some windows that are actually too far to contribute to the
	k closest points
	
	Now we just have to find whether x1, x2 is closer to l1, l2, r1, r2, or is between them
	*/
	const cr8r_kdwin_s2i64 *self = _self;
	const int64_t *pt = _pt;
	cr8r_kd_ft ft = *_ft;
	double sqdist = -1;
	ft.super.base.data = (void*)(ft.dim - 1);
	if(ft.super.cmp(&ft.super.base, self->bl, self->tr) > 0){
		if(ft.super.cmp(&ft.super.base, self->bl, pt) <= 0){
			sqdist = 0;
		}
		if(ft.super.cmp(&ft.super.base, pt, self->tr) <= 0){
			sqdist = 0;
		}
	}else if(!self->bl[1] && !self->tr[1]){
		sqdist = 0;
	}else{
		if(ft.super.cmp(&ft.super.base, self->bl, pt) <= 0 && ft.super.cmp(&ft.super.base, pt, self->tr) <= 0){
			sqdist = 0;
		}
	}
	if(sqdist == -1){
		int64_t lcrs = self->bl[0]*self->bl[0] + self->bl[1]*self->bl[1];
		int64_t rcrs = self->tr[0]*self->tr[0] + self->tr[1]*self->tr[1];
		int64_t acrs = pt[0]*pt[0] + pt[1]*pt[1];
		int64_t lda = pt[0]*self->bl[0] + pt[1]*self->bl[1];
		int64_t rda = pt[0]*self->tr[0] + pt[1]*self->tr[1];
		if(lcrs*rda < rcrs*lda){
			sqdist = 2*(acrs - (double)rda/rcrs);
		}else{
			sqdist = 2*(acrs - (double)lda/lcrs);
		}
	}
	for(uint64_t i = 2; i <= ft.dim; ++i){
		int64_t axdist = self->bl[i] - pt[i];
		if(pt[i] - self->tr[i] > axdist){
			axdist = pt[i] - self->tr[i];
		}
		sqdist += axdist > 0 ? axdist*axdist : 0;
	}
	return sqdist;
}

static double sqdist_i64sp(const cr8r_kd_ft *_ft, const void *_a, const void *_b){
	double res = 0;
	const int64_t *a = _a, *b = _b;
	for(uint64_t i = 0; i < _ft->dim + 1; ++i){
		res += (a[i] - b[i])*(a[i] - b[i]);
	}
	return res;
}



static void split_i64cu(const cr8r_kd_ft *ft, const void *_self, const void *_root_pt, void *_o1, void *_o2){
	const cr8r_kdwin_s2i64 *self = _self;
	const int64_t *root = _root_pt;
	cr8r_kdwin_s2i64 *o1 = _o1;
	cr8r_kdwin_s2i64 *o2 = _o2;
	uint64_t idx = (uint64_t)ft->super.base.data%ft->dim;
	memcpy(o1, self, ft->dim*sizeof(int64_t));
	memcpy(o2, self, ft->dim*sizeof(int64_t));
	o1->tr[idx] = root[idx];
	o2->bl[idx] = root[idx];
}

static void update_i64cu(const cr8r_kd_ft *_ft, void *_self, const void *_pt){
	cr8r_kdwin_s2i64 *self = _self;
	const int64_t *pt = _pt;
	cr8r_kd_ft ft = *_ft;
	for(uint64_t i = 0; i < ft.dim; ++i){
		ft.super.base.data = (void*)i;
		if(ft.super.cmp(&ft.super.base, pt, self->bl) < 0){
			self->bl[i] = pt[i];
		}else if(ft.super.cmp(&ft.super.base, pt, self->tr) > 0){
			self->tr[i] = pt[i];
		}
	}
}

static double min_sqdist_i64cu(const cr8r_kd_ft *_ft, const void *_self, const void *_pt){
	const cr8r_kdwin_s2i64 *self = _self;
	const int64_t *pt = _pt;
	double sqdist = 0;
	for(uint64_t i = 0; i < _ft->dim; ++i){
		int64_t axdist = self->bl[i] - pt[i];
		if(pt[i] - self->tr[i] > axdist){
			axdist = pt[i] - self->tr[i];
		}
		sqdist += axdist > 0 ? axdist*axdist : 0;
	}
	return sqdist;
}

static double sqdist_i64cu(const cr8r_kd_ft *_ft, const void *_a, const void *_b){
	double res = 0;
	const int64_t *a = _a, *b = _b;
	for(uint64_t i = 0; i < _ft->dim; ++i){
		res += (a[i] - b[i])*(a[i] - b[i]);
	}
	return res;
}



bool cr8r_kdwin_init_i64sp(cr8r_kdwin_s2i64 *self, const int64_t bl[3], const int64_t tr[3]){
	memcpy(self->bl, bl, 3*sizeof(int64_t));
	memcpy(self->tr, tr, 3*sizeof(int64_t));
	return 1;
}

bool cr8r_kdwin_init_i64cu(cr8r_kdwin_s2i64 *self, const int64_t bl[3], const int64_t tr[3]){
	memcpy(self->bl, bl, 3*sizeof(int64_t));
	memcpy(self->tr, tr, 3*sizeof(int64_t));
	return 1;
}

bool cr8r_kdwin_bounding_i64x3(cr8r_kdwin_s2i64 *self, const cr8r_vec *ents, const cr8r_kd_ft *ft){
	if(!ents->len){
		return 0;
	}
	memcpy(self->bl, ents->buf, ft->super.base.size);
	memcpy(self->tr, ents->buf, ft->super.base.size);
	for(uint64_t i = 1; i < ents->len; ++i){
		const void *ent = ents->buf + i*ft->super.base.size;
		ft->update(ft, self, ent);
	}
	return 1;
}

bool cr8r_kd_ify(cr8r_vec *self, cr8r_kd_ft *_ft, uint64_t a, uint64_t b){
	cr8r_kd_ft ft = *_ft;
	while(b > a){
		uint64_t mid_idx = (a + b)/2;
		void *piv = cr8r_vec_ith(self, &ft.super, a, b, mid_idx - a);
		if(!piv){
			return 0;
		}
		piv = cr8r_vec_partition_with_median(self, &ft.super, a, b, piv);
		// increment depth
		++*(uint64_t*)&ft.super.base.data;
		if(!cr8r_kd_ify(self, &ft, mid_idx + 1, b)){
			return 0;
		}
		b = mid_idx;
	}
	return 1;
}

cr8r_walk_decision cr8r_kd_walk_r(cr8r_vec *self, const cr8r_kd_ft *_ft, void *bounds, cr8r_kdvisitor visitor, void *data, uint64_t a, uint64_t b){
	cr8r_kd_ft ft = *_ft;
	char sub0[ft.bounds_size];
	char sub1[ft.bounds_size];
	while(b > a){
		uint64_t mid_idx = (a + b)/2;
		void *ent =  self->buf + mid_idx*ft.super.base.size;
		cr8r_walk_decision decision = visitor(&ft, bounds, ent, data);
		if(decision == CR8R_WALK_STOP){
			return decision;
		}else if(decision == CR8R_WALK_SKIP_CHILDREN){
			return CR8R_WALK_CONTINUE;
		}
		ft.split(&ft, bounds, ent, sub0, sub1);
		// increment depth
		++*(uint64_t*)&ft.super.base.data;
		decision = cr8r_kd_walk_r(self, &ft, sub0, visitor, data, a, mid_idx);
		if(decision == CR8R_WALK_STOP){
			return decision;
		}
		a = mid_idx + 1;
		memcpy(bounds, sub1, ft.bounds_size);
	}
	return CR8R_WALK_CONTINUE;
}

void cr8r_kd_walk(cr8r_vec *self, const cr8r_kd_ft *ft, const void *_bounds, cr8r_kdvisitor visitor, void *data){
	char bounds[ft->bounds_size];
	memcpy(bounds, _bounds, ft->bounds_size);
	cr8r_kd_walk_r(self, ft, bounds, visitor, data, 0, self->len);
}

typedef struct{
	cr8r_vec *ents;
	cr8r_kd_ft ft;
	const void *pt;
	uint64_t k;
	double max_sqdist;
} k_closest_state;

inline static cr8r_walk_decision k_closest_visitor(cr8r_kd_ft *ft, const void *bounds, void *ent, void *_data){
	k_closest_state *data = _data;
	char tmp[ft->super.base.size];
	if(data->ents->len < data->k){
		cr8r_mmheap_push(data->ents, &data->ft.super, ent);
	}else{
		cr8r_mmheap_pushpop_max(data->ents, &data->ft.super, ent, tmp);
		data->max_sqdist = ft->sqdist(ft, data->pt, cr8r_mmheap_peek_max(data->ents, &data->ft.super));
	}
	if(isinf(data->max_sqdist) || data->max_sqdist > ft->min_sqdist(ft, bounds, ent)){
		return CR8R_WALK_CONTINUE;
	}
	return CR8R_WALK_SKIP_CHILDREN;
}

inline static int cmp_pt_dist(const cr8r_base_ft *_ft, const void *a, const void *b){
	const cr8r_kd_ft *ft = (const cr8r_kd_ft*)_ft;
	const k_closest_state *data = CR8R_OUTER(ft, k_closest_state, ft);
	double a_sqdist = ft->sqdist(ft, data->pt, a);
	double b_sqdist = ft->sqdist(ft, data->pt, b);
	if(a_sqdist < b_sqdist){
		return -1;
	}else if(a_sqdist > b_sqdist){
		return 1;
	}
	return 0;
}

void cr8r_kd_k_closest(cr8r_vec *self, cr8r_kd_ft *ft, const void *bounds, const void *pt, uint64_t k, cr8r_vec *out){
	k_closest_state data = {
		.ents = out,
		.ft = *ft,
		.pt = pt,
		.k = k,
		.max_sqdist = INFINITY
	};
	data.ft.super.cmp = cmp_pt_dist;
	cr8r_kd_walk(self, ft, bounds, k_closest_visitor, &data);
}

void cr8r_kd_k_closest_naive(cr8r_vec *self, cr8r_kd_ft *ft, const void *bounds, const void *pt, uint64_t k, cr8r_vec *out){
	k_closest_state data = {
		.ents = out,
		.ft = *ft,
		.pt = pt,
		.k = k,
		.max_sqdist = INFINITY
	};
	data.ft.super.cmp = cmp_pt_dist;
	for(uint64_t i = 0; i < self->len; ++i){
		k_closest_visitor(ft, bounds, self->buf + i*ft->super.base.size, &data);
	}
}

cr8r_kd_ft cr8r_kdft_s2i64 = {
	.super.base.size = 3*sizeof(int64_t),
	.super.new_size = cr8r_default_new_size,
	.super.resize = cr8r_default_resize,
	.super.cmp = cmp_depth_i64sp,
	.super.swap = cr8r_default_swap,
	.dim = 2,
	.bounds_size = 6*sizeof(int64_t),
	.split = split_i64sp,
	.update = update_i64sp,
	.min_sqdist = min_sqdist_i64sp,
	.sqdist = sqdist_i64sp
};

cr8r_kd_ft cr8r_kdft_c3i64 = {
	.super.base.size = 3*sizeof(int64_t),
	.super.new_size = cr8r_default_new_size,
	.super.resize = cr8r_default_resize,
	.super.cmp = cmp_depth_i64cu,
	.super.swap = cr8r_default_swap,
	.dim = 3,
	.bounds_size = 6*sizeof(int64_t),
	.split = split_i64cu,
	.update = update_i64cu,
	.min_sqdist = min_sqdist_i64cu,
	.sqdist = sqdist_i64cu
};

