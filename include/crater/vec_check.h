#pragma once

#include <crater/vec.h>

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

