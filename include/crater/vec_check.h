#pragma once

#include <crater/vec.h>

inline static bool cr8r_vec_check_partition(const cr8r_vec *self, const cr8r_vec_ft *ft, uint64_t a, uint64_t b, const void *piv){
	const void *fst = self->buf + a*ft->base.size;
	const void *lst = self->buf + (b - 1)*ft->base.size;
	if(a >= b || b > self->len || piv < fst || piv > lst){
		return false;
	}
	for(const void *it = fst; it < piv; it += ft->base.size){
		if(ft->cmp(&ft->base, it, piv) >= 0){
			return false;
		}
	}
	for(const void *it = piv; it <= lst; it += ft->base.size){
		if(ft->cmp(&ft->base, it, piv) < 0){
			return false;
		}
	}
	return true;
}

inline static bool cr8r_vec_check_pwm(cr8r_vec_ft *ft, cr8r_vec *self, uint64_t a, uint64_t lb, uint64_t ha, uint64_t b, void *med){
	for(uint64_t i = a; i < lb; ++i){
		if(ft->cmp(&ft->base, self->buf + i*ft->base.size, med) >= 0){
			return false;
		}
	}
	for(uint64_t i = lb; i < ha; ++i){
		if(ft->cmp(&ft->base, self->buf + i*ft->base.size, med)){
			return false;
		}
	}
	for(uint64_t i = ha; i < b; ++i){
		if(ft->cmp(&ft->base, self->buf + i*ft->base.size, med) <= 0){
			return false;
		}
	}
	return true;
}

