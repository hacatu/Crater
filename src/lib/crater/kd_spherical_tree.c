#include "crater/vec.h"
#include <crater/kd_tree.h>
#include <string.h>

/// Return an identifier for the region of the xy plane containing a point
///
/// Returns 0 for origin or 1 to 8 for the +/- axis and quadrants in
/// counterclockwise order starting from the +x axis (ie 1 for +x axis,
/// 2 for quadrant I, etc, and 8 for quadrant IV)
static inline int find_x_y_region(const int64_t *a){
	if(a[0] == 0){// a is on the y axis
		if(a[1] == 0){// a is the origin
			return 0;
		}
		return a[1] > 0 ? 3 : 7;// +y axis and -y axis respectively
	}else if(a[1] == 0){// a is on the x axis (but not the origin)
		return a[0] > 0 ? 1 : 5;// +x axis and -x axis respectively
	}else if(a[1] > 0){// a is in the upper half plane
		return a[0] > 0 ? 2 : 4;// quadrant I and quadrant II respectively
	}// otherwise a is in the lower half plane
	return a[0] > 0 ? 8 : 6;// quadrant IV and quadrant III respectively
}

static inline int cmp_x_y(const cr8r_base_ft *base, const void *_a, const void *_b){
	const int64_t *a = _a, *b = _b;
	// consider the points a and b projected into the xy plane (ie ignore their z components)
	// we want to find which point, a or b, has a smaller angle in its standard polar representation
	// return -1 if a has a smaller angle, 1 if b does, 0 if they have the same angle
	// we do not need to do trig for this, but there are a lot of cases
	// for a fixed z, all xy points have a fixed radius, so comparison of points at the same z
	// can be done instantly with one coordinate.  However, when a and b have different z coordinates we
	// need to work harder.
	int a_region = find_x_y_region(a), b_region = find_x_y_region(b);
	if(a_region == 0 || b_region == 0){
		return 0;
	}else if(a_region < b_region){
		return -1;
	}else if(a_region > b_region){
		return 1;
	}else if(a_region & 1){// both regions are the same, if they are odd both points are on the same axis (+/- x/y)
		return 0;
	}// otherwise both points are in the same quadrant
	int64_t ord = b[0]*a[1] - a[0]*b[1];
	if(ord < 0){
		return -1;
	}else if(ord == 0){
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

int64_t cr8r_kd_i64x3_sqdist(const int64_t a[static 3], const int64_t b[static 3]){
	int64_t sqdist = 0;
	for(int64_t i = 0; i < 3; ++i){
		int64_t tmp = a[i] - b[i];
		sqdist += tmp*tmp;
	}
	return sqdist;
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

static inline bool iqq_contains_concave_check_axis(int64_t max_r2, bool split_crosses_axis, int64_t qa, int64_t qb, int64_t wm){
	return !(
		(max_r2 > qb*qb) ||
		(split_crosses_axis ?
			wm*wm > qa*qa:
			max_r2 > qa*qa)
	);
}

static inline bool iqq_contains_concave_check_xy(int64_t max_r2, cr8r_kd_s2i64_quad query, cr8r_kd_s2i64_quad window){
	// in this case, the z projection of window is contained in the z projection of query
	// and window has an inner angle of at least a half turn, so the aabb of window is determined
	// only by the outer arc
	if(query.a[0]*query.b[0] > 0 && query.a[1]*query.b[1] > 0){
		return false;// the z axis does not go through the query region so we can only have a partial intersection
	}
	int64_t split_x = window.a[0] + window.b[0];
	int64_t split_y = window.a[1] + window.b[1];
	bool split_crosses_x = window.a[1]*window.b[1] < 0;
	bool split_crosses_y = window.a[0]*window.b[0] < 0;
	// check top, bottom, left, and right for containment
	int64_t qa = (split_x <= 0 ? query.a : query.b)[0];
	int64_t qb = (split_x <= 0 ? query.b : query.a)[0];
	int64_t wm = (split_x <= 0) == (window.a[0] <= window.b[0]) ? window.a[0] : window.b[0];
	if(!iqq_contains_concave_check_axis(max_r2, split_crosses_x, qa, qb, wm)){
		return false;
	}
	qa = (split_y <= 0 ? query.a : query.b)[1];
	qb = (split_y <= 0 ? query.b : query.a)[1];
	wm = (split_y <= 0) == (window.a[1] <= window.b[1]) ? window.a[1] : window.b[1];
	return iqq_contains_concave_check_axis(max_r2, split_crosses_y, qa, qb, wm);
}

int cr8r_kd_s2i64_intersect_quad_quad(int64_t r2, cr8r_kd_s2i64_quad query, cr8r_kd_s2i64_quad window){
	// first we check if the regions overlap in the z axis
	if(query.b[2] > window.a[2] || window.b[2] > query.a[2]){
		return 0;
	}
	// if they do, we can work only in the overlapping z region
	// we can then project into the xy plane, by finding the arcs with the smallest and largest radii
	int res = 2;
	int64_t z_max = window.a[2], z_min = window.b[2];
	if(query.a[2] < window.a[2]){
		z_max = query.a[2];
		res = 1;
	}
	if(query.b[2] > window.b[2]){
		z_min = query.b[2];
		res = 1;
	}
	int64_t min_r_z = z_max*z_max > z_min*z_min ? z_min : z_max;
	int64_t max_r_z = z_max*z_min <= 0 ? 0 : min_r_z == z_min ? z_max : z_min;
	int64_t max_r2 = r2 - max_r_z*max_r_z;
	// now min_r_z is the z coordinate on the z region of quad (which is on the sphere) where
	// the xy slice of the sphere has minimal radius,
	// and similarly max_r_z is the z coordinate leading to maximal radius
	bool is_convex_angle = window.a[0]*window.b[1] > window.a[1]*window.b[0];
	// if the angle subtended around the z axis by window is less than a half turn, the inner arc
	// matters for strict inclusion, otherwise it doesn't, because it cannot come in contact with
	// the axis aligned bounding box of window in the xy projection.
	while(res == 2){// this is not a loop
		if(!is_convex_angle){
			if(iqq_contains_concave_check_xy(max_r2, query, window)){
				return 2;
			}
		}else{
			// otherwise, the z projection of window is contained in the z projection of query
			// but window has an inner angle of less than half a turn, so the aabb of window is
			// determined by both the outer and inner arcs 
			// TODO: finish this function
			return 1;
		}
	}
	// TODO: finish this function
	return 1;
}

static inline void kd_s2i64_forall_quad_r(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft,
	cr8r_kd_s2i64_quad query, uint64_t a, uint64_t b,
	void *data, void (*f)(void *data, const void *point)
){
	while(b > a){
		uint64_t mid_idx = (a + b)/2;
		int (*next_cmp)(const cr8r_base_ft*, const void*, const void*) = ft->cmp == cmp_z ? cmp_x_y : cmp_z;
		void *piv = cr8r_vec_get(&self->points, ft, mid_idx);
		bool visit_lt_part = ft->cmp(&ft->base, piv, query.a) >= 0;
		bool visit_ge_part = ft->cmp(&ft->base, piv, query.b) <= 0;
		if(visit_lt_part && visit_ge_part){
			ft->cmp = next_cmp;
			// TODO: one or both of these checks can be redundant based on the caller's pivot.
			// If we add an extra argument to this function we can remove these checks sometimes.
			if(ft->cmp(&ft->base, piv, query.a) >= 0 && ft->cmp(&ft->base, piv, query.b) <= 0){
				f(data, piv);
			}
			kd_s2i64_forall_quad_r(self, ft, query, mid_idx + 1, b, data, f);
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

void cr8r_kd_s2i64_forall_quad(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, cr8r_kd_s2i64_quad query, void *data, void (*f)(void *data, const void *point)){
	cr8r_vec_ft cft = *ft;
	cft.cmp = cmp_z;
	kd_s2i64_forall_quad_r(self, &cft, query, 0, self->points.len, data, f);
}

int cr8r_kd_s2i64_intersect_ball_quad(int64_t r2, cr8r_kd_s2i64_ball query, cr8r_kd_s2i64_quad window){
	int64_t D2 = query.sqdist;
	// first, we check if both corners of the window region are in the query region
	// if only one is, we know we have a partial intersection without going to the next step
	if(cr8r_kd_i64x3_sqdist(query.p, window.a) <= D2){
		return cr8r_kd_i64x3_sqdist(query.p, window.b) <= D2 ? 2 : 1;
	}
	// if neither corner of the window region is in the query region, we check if the query region intersects
	// the surface of the sphere at all.  if p is the distance to the center of the query region, r is the radius
	// of the sphere, and D is the maximum allowable distance, we can be sure there is no intersection if
	// p > r + D
	// p**2 > (r + D)**2
	// p**2 - r**2 - D**2 > 2*r*D
	// p**2 > r**2 + D**2 and (p**2 - r**2 - D**2)**2 > 4*r**2*D**2
	// p**2 > r**2 + D**2 and p**4 + r**4 + D**4 - 2*p**2*(r**2 + D**2) > 2*r**2*D**2
	// on the other hand, the query center could be inside the sphere, in which case there is no intersection
	// when D < r and p**2 < (r - D)**2, which we can expand similarly to above to only involve squares
	// we then notice something interesting, the condition on the squares is the same in both the interior
	// and exterior cases so we can lift it outside of both checks, as in the code below
	//
	// first, we compute p**2
	int64_t p2 = cr8r_kd_i64x3_sqdist(query.p, ((int64_t[3]){}));
	// now, we check the condition on the squares
	if(p2*(p2 - 2*r2 - 2*D2) + r2*r2 > (2*r2 - D2)*D2){
		// if the condition on the squares holds, we check if either the precondition
		// for the external centered query region or internal centered query region holds
		if(p2 > r2 + D2 || (D2 < r2 && p2 < r2 + D2)){
			return 0;
		}
	}
	// whether the projection of the centerpoint of the query region is
	// in the upper hemisphere, within the angle OR band of the window region,
	// within the band of the window region respectively
	// NOTE that the upper hemisphere is NEGATIVE
	bool is_upper = false, is_orth = false, in_band = false;
	int64_t tr[3] = {window.b[0], window.b[1], window.a[2]};
	int64_t bl[3] = {window.a[0], window.a[1], window.b[2]};
	if(cmp_z(NULL, query.p, window.a) < 0){
		is_upper = true;
		is_orth ^= true;
	}else if(cmp_z(NULL, query.p, window.b) > 0){
		is_upper = false;
		is_orth ^= true;
	}else{
		in_band = true;
	}
	if(cmp_x_y(NULL, query.p, window.a) < 0 || (window.b[0] && cmp_x_y(NULL, query.p, window.b) > 0)){
		is_orth ^= true;
		if(in_band && (2*query.p[2] < window.a[2] + window.b[2])){
			is_upper = true;
		}
	}
	if(in_band && !is_orth){
		return 1;
	}
	bool is_right = cr8r_kd_i64x3_sqdist(query.p, tr) < cr8r_kd_i64x3_sqdist(query.p, window.a);
	int64_t nearest_pt[3];
	if(!is_orth){
		memcpy(nearest_pt, is_right ?
			is_upper ? tr : window.b :
			is_upper ? window.a : bl, 3*sizeof(int64_t));
	}else if(in_band){
		memcpy(nearest_pt, is_right ? window.b : window.a, 2*sizeof(int64_t));
		nearest_pt[2] = query.p[2];
	}else{// otherwise the query region center projection is orthogonally above or below the window
		// so the point on the boundary of the window that is nearest to the query region center is
		// (L*p_x, L*p_y, a_z) where L = sqrt(r**2 - a_z**2)/sqrt(p**2 - p_z**2)
		// note that if the projection is BELOW instead of above, replace a with b.
		// thus we want p**2 - 2*sqrt((r**2 - a_z**2)*(p**2 - p_z**2)) + r**2 - 2*p_z*a_z <= D**2
		// which is true if either
		// p**2 + r**2 - D**2 - 2*p_z*a_z <= 0 OR
		// p**4 + r**4 - 2*p**2*D**2 - 2*r**2*D**2 - 4*p**2*p_z*a_z - 4*r**2*p_z*a_z + D**4 + 4*D**2*p_z*a_z <= 2*r**2*p**2 - 4*r**2*p_z**2 - 4*p**2*a_z**2
		memcpy(nearest_pt, is_upper ? window.a : window.b, 3*sizeof(int64_t));
		if(p2 + r2 - D2 - 2*query.p[2]*nearest_pt[2] <= 0){
			return 1;
		}else if(p2*(p2 + 4*nearest_pt[2]*nearest_pt[2]) + r2*(r2 + 4*query.p[2]*query.p[2]) + D2*(D2 - 2*p2 - 2*r2) - 4*query.p[2]*nearest_pt[2]*(p2 + r2 - D2) <= 2*r2*p2){
			return 1;
		}
		return 0;
	}
	return cr8r_kd_i64x3_sqdist(query.p, nearest_pt) <= D2;
}

static inline void kd_s2i64_forall_r(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b, void *data, void (*f)(void *data, const void *point)){
	if(a < b){
		uint64_t mid_idx = (a + b)/2;
		cr8r_kd_i64x3_pt *piv = cr8r_vec_get(&self->points, ft, mid_idx);
		f(data, piv);
		kd_s2i64_forall_r(self, ft, a, mid_idx, data, f);
		kd_s2i64_forall_r(self, ft, mid_idx + 1, b, data, f);
	}
}

static inline void kd_s2i64_forall_ball_r(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, cr8r_kd_s2i64_ball query, cr8r_kd_s2i64_quad window, uint64_t a, uint64_t b, void *data, void (*f)(void *data, const void *point)){
	if(a < b){
		uint64_t mid_idx = (a + b)/2;
		int (*next_cmp)(const cr8r_base_ft*, const void*, const void*) = ft->cmp == cmp_z ? cmp_x_y : cmp_z;
		cr8r_kd_i64x3_pt *piv = cr8r_vec_get(&self->points, ft, mid_idx);
		cr8r_kd_s2i64_quad first_subwindow = window;
		cr8r_kd_s2i64_quad second_subwindow = window;
		if(ft->cmp == cmp_z){
			first_subwindow.b[2] = piv->coords[2];
			second_subwindow.a[2] = piv->coords[2];
		}else{
			memcpy(first_subwindow.b, piv->coords, 2*sizeof(int64_t));
			memcpy(second_subwindow.a, piv->coords, 2*sizeof(int64_t));
		}
		if(cr8r_kd_i64x3_sqdist(query.p, piv->coords) <= query.sqdist){
			f(data, piv);
		}
		int64_t r2 = self->radius*self->radius;
		int first_status = cr8r_kd_s2i64_intersect_ball_quad(r2, query, first_subwindow);
		int second_status = cr8r_kd_s2i64_intersect_ball_quad(r2, query, second_subwindow);
		ft->cmp = next_cmp;
		// TODO: right now we are letting the compiler earn its paycheck by performing tail call optimization on this
		// function.  Consider applying the optimization by hand so that it will exist in debug builds.
		if(first_status == 1){
			kd_s2i64_forall_ball_r(self, ft, query, first_subwindow, a, mid_idx, data, f);
		}else if(first_status == 2){
			kd_s2i64_forall_r(self, ft, a, mid_idx, data, f);
		}
		ft->cmp = next_cmp;
		if(second_status == 1){
			kd_s2i64_forall_ball_r(self, ft, query, second_subwindow, mid_idx + 1, b, data, f);
		}else if(second_status == 2){
			kd_s2i64_forall_r(self, ft, mid_idx + 1, b, data, f);
		}
	}
}

void cr8r_kd_s2i64_forall_ball(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, cr8r_kd_s2i64_ball query, void *data, void (*f)(void *data, const void *point)){
	cr8r_vec_ft cft = *ft;
	cft.cmp = cmp_z;
	cr8r_kd_s2i64_quad window = {.a={0, 0, -self->radius}, .b={0, 0, self->radius}};
	kd_s2i64_forall_ball_r(self, &cft, query, window, 0, self->points.len, data, f);
}

void cr8r_kd_s2i64_forall(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, void *data, void (*f)(void *data, const void *point)){
	cr8r_vec_ft cft = *ft;
	cft.cmp = cmp_z;
	kd_s2i64_forall_r(self, &cft, 0, self->points.len, data, f);
}

