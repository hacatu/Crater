#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>

#include <crater/heap.h>

void cr8r_heap_ify(cr8r_vec *self, cr8r_vec_ft *ft, int ord){
	for(uint64_t i = (ULLONG_MAX >> 1) >> __builtin_clzll(self->len); i-- > 0;){
		cr8r_heap_sift_down(self, ft, self->buf + i*ft->base.size, ord);//void pointer arithmetic works like char pointer arithmetic
	}
}

void *cr8r_heap_top(cr8r_vec *self, cr8r_vec_ft *ft){
	return self->len ? self->buf : NULL;
}

void cr8r_heap_sift_up(cr8r_vec *self, cr8r_vec_ft *ft, void *e, int ord){
	uint64_t i = (e - self->buf)/ft->base.size;
	uint64_t j = (i - 1) >> 1;
	while(i && ord*ft->cmp(&ft->base, self->buf + i*ft->base.size, self->buf + j*ft->base.size) > 0){
		ft->swap(&ft->base, self->buf + i*ft->base.size, self->buf + j*ft->base.size);
		i = j;
		j = (i - 1) >> 1;
	}
}

void cr8r_heap_sift_down(cr8r_vec *self, cr8r_vec_ft *ft, void *e, int ord){
	uint64_t i = (e - self->buf)/ft->base.size;
	uint64_t l = (i << 1) + 1, r = l + 1, j;
	while(r < self->len){
		j = ord*ft->cmp(&ft->base, self->buf + l*ft->base.size, self->buf + r*ft->base.size) >= 0 ? l : r;
		if(ord*ft->cmp(&ft->base, self->buf + i*ft->base.size, self->buf + j*ft->base.size) >= 0){
			return;
		}
		ft->swap(&ft->base, self->buf + i*ft->base.size, self->buf + j*ft->base.size);
		i = j;
		l = (i << 1) + 1, r = l + 1;
	}
	if(l < self->len && ord*ft->cmp(&ft->base, self->buf + i*ft->base.size, self->buf + l*ft->base.size) < 0){
		ft->swap(&ft->base, self->buf + i*ft->base.size, self->buf + l*ft->base.size);
	}
}

bool cr8r_heap_push(cr8r_vec *self, cr8r_vec_ft *ft, const void *e, int ord){
	if(!cr8r_vec_pushr(self, ft, e)){
		return 0;
	}
	cr8r_heap_sift_up(self, ft, self->buf + (self->len - 1)*ft->base.size, ord);
	return 1;
}

bool cr8r_heap_pop(cr8r_vec *self, cr8r_vec_ft *ft, void *o, int ord){
	if(self->len > 1){
		ft->swap(&ft->base, self->buf, self->buf + (self->len - 1)*ft->base.size);
	}
	if(!cr8r_vec_popr(self, ft, o)){
		return 0;
	}
	if(self->len){
		cr8r_heap_sift_down(self, ft, self->buf, ord);
	}
	return 1;
}

