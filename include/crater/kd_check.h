#pragma once

#include <crater/kd_tree.h>

inline static bool cr8r_kd_check_tree(const cr8r_vec *self, const cr8r_kd_ft *_ft, uint64_t a, uint64_t b){
	cr8r_kd_ft ft = *_ft;
	while(b > a){
		uint64_t mid_idx = (a + b)/2;
		const void *mid = self->buf + mid_idx*ft.super.base.size;
		for(uint64_t i = a; i < mid_idx; ++i){
			const void *ent = self->buf + i*ft.super.base.size;
			if(ft.super.cmp(&ft.super.base, ent, mid) > 0){
				return false;
			}
		}
		for(uint64_t i = mid_idx + 1; i < b; ++i){
			const void *ent = self->buf + i*ft.super.base.size;
			if(ft.super.cmp(&ft.super.base, ent, mid) < 0){
				return false;
			}
		}
		// increment depth
		++*(uint64_t*)&ft.super.base.data;
		if(!cr8r_kd_check_tree(self, &ft, mid_idx + 1, b)){
			return false;
		}
		b = mid_idx;
	}
	return true;
}

