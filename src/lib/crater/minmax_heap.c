#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>

#include <crater/minmax_heap.h>

void cr8r_mmheap_ify(cr8r_vec *self, cr8r_vec_ft *ft){
	for(uint64_t i = (ULLONG_MAX >> 1) >> __builtin_clzll(self->len); i-- > 0;){
		cr8r_mmheap_sift_down(self, ft, self->buf + i*ft->base.size);//void pointer arithmetic works like char pointer arithmetic
	}
}

void *cr8r_mmheap_peek_min(cr8r_vec *self, cr8r_vec_ft *ft){
	return self->len ? self->buf : NULL;
}

void *cr8r_mmheap_peek_max(cr8r_vec *self, cr8r_vec_ft *ft){
	if(self->len < 3){
		return self->len ? self->buf + (self->len - 1)*ft->base.size : NULL;
	}
	void *l = self->buf + 1*ft->base.size, *r = self->buf + 2*ft->base.size;
	return ft->cmp(&ft->base, l, r) > 0 ? l : r;
}

void cr8r_mmheap_sift_up(cr8r_vec *self, cr8r_vec_ft *ft, void *e){
	if(e == self->buf){
		return;
	}
	uint64_t i = (e - self->buf)/ft->base.size;
	// the bit length of i + 1 gives a good way to compute the ceil of the
	// binary log of i + 2, which is the height of the shortest complete
	// heap containing an ith element.  If there is an odd number of layers,
	// i is in a min layer, otherwise it is in a max layer.  Finally,
	// note the parity of the bit length of i + 1 is the same as the parity of
	// the number of leading zeros of i + 1.
	bool is_min_layer = __builtin_clzll(i + 1)&1;
	uint64_t i1 = (i - 1) >> 1;
	void *e1 = self->buf + i1*ft->base.size;
	int ord = is_min_layer ? 1 : -1;
	if(ord*ft->cmp(&ft->base, e1, e) < 0){
		ft->swap(&ft->base, e, e1);
		i = i1;
		e = e1;
		is_min_layer ^= 1;
		ord = -ord;
	}
	while(i > 2){
		i1 = (i - 3) >> 2;
		e1 = self->buf + i1*ft->base.size;
		if(ord*ft->cmp(&ft->base, e, e1) < 0){
			ft->swap(&ft->base, e, e1);
			i = i1;
			e = e1;
		}else{
			break;
		}
	}
}

void cr8r_mmheap_sift_down(cr8r_vec *self, cr8r_vec_ft *ft, void *e){
	uint64_t i = (e - self->buf)/ft->base.size;
	bool is_min_layer = __builtin_clzll(i + 1)&1;
	int ord = is_min_layer ? 1 : -1;
	while(2*i + 1 < self->len){
		// Find m and em, the index of the extremal element and a pointer to the extremal element
		// respectively among the children and grandchildren of e.  For min layers, extremal means
		// minimal, and for max layers it means maximal
		uint64_t m = 2*i + 1;
		void *em = self->buf + m*ft->base.size;
		const uint64_t idxs[] = {2*i + 2, 4*i + 3, 4*i + 4, 4*i + 5, 4*i + 6};
		for(uint64_t ii = 0; ii < sizeof(idxs)/sizeof(uint64_t) && idxs[ii] < self->len; ++ii){
			void *ei = self->buf + idxs[ii]*ft->base.size;
			if(ord*ft->cmp(&ft->base, ei, em) < 0){
				m = idxs[ii];
				em = ei;
			}
		}
		// If em is a grandchild of e (as should be the case most of the time)
		// we may have to sift down farther after fixing up here
		if(m > 2*i + 2){
			if(ord*ft->cmp(&ft->base, em, e) < 0){
				ft->swap(&ft->base, em, e);
				uint64_t p = (m - 1) >> 1;
				void *ep = self->buf + p*ft->base.size;
				if(ord*ft->cmp(&ft->base, ep, em) < 0){
					ft->swap(&ft->base, em, ep);
				}
				i = m;
				e = em;
			}else{
				break;
			}
		}else{// otherwise em is a direct child so it must be a leaf or its invariant would be wrong
			if(ord*ft->cmp(&ft->base, em, e) < 0){
				ft->swap(&ft->base, em, e);
			}
			break;
		}
	}
}

bool cr8r_mmheap_push(cr8r_vec *self, cr8r_vec_ft *ft, const void *e){
	if(!cr8r_vec_pushr(self, ft, e)){
		return false;
	}
	cr8r_mmheap_sift_up(self, ft, cr8r_vec_getx(self, ft, -1));
	return true;
}

bool cr8r_mmheap_pop_min(cr8r_vec *self, cr8r_vec_ft *ft, void *o){
	return cr8r_mmheap_pop_idx(self, ft, o, 0);
}

bool cr8r_mmheap_pop_max(cr8r_vec *self, cr8r_vec_ft *ft, void *o){
	if(self->len < 3){
		return self->len ? cr8r_mmheap_pop_idx(self, ft, o, self->len - 1) : false;
	}
	void *l = self->buf + 1*ft->base.size, *r = self->buf + 2*ft->base.size;
	return cr8r_mmheap_pop_idx(self, ft, o, ft->cmp(&ft->base, l, r) > 0 ? 1 : 2);
}

bool cr8r_mmheap_pop_idx(cr8r_vec *self, cr8r_vec_ft *ft, void *o, uint64_t i){
	if(i == self->len - 1){
		return cr8r_vec_popr(self, ft, o);
	}else if(!self->len){
		return false;
	}
	void *e = self->buf + i*ft->base.size;
	memcpy(o, e, ft->base.size);
	cr8r_vec_popr(self, ft, e);
	cr8r_mmheap_sift_down(self, ft, e);
	return true;
}

bool cr8r_mmheap_pushpop_min(cr8r_vec *self, cr8r_vec_ft *ft, const void *e, void *o){
	if(!cr8r_mmheap_push(self, ft, e)){
		return false;
	}
	return cr8r_mmheap_pop_min(self, ft, o);
}

bool cr8r_mmheap_pushpop_max(cr8r_vec *self, cr8r_vec_ft *ft, const void *e, void *o){
	if(!cr8r_mmheap_push(self, ft, e)){
		return false;
	}
	return cr8r_mmheap_pop_max(self, ft, o);
}

