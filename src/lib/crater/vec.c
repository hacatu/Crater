#include <stdlib.h>
#include <string.h>
#include <sys/random.h>

#include <crater/vec.h>
#include <crater/heap.h>


bool cr8r_vec_ft_init(cr8r_vec_ft *ft,
	void *data, uint64_t size,
	uint64_t (*new_size)(cr8r_base_ft*, uint64_t cap),
	void *(*resize)(cr8r_base_ft*, void *p, uint64_t cap),
	void (*del)(cr8r_base_ft*, void *p),
	void (*copy)(cr8r_base_ft*, void *dest, const void *src),
	int (*cmp)(const cr8r_base_ft*, const void *a, const void *b),
	void (*swap)(cr8r_base_ft*, void *a, void *b)
){
	if(!resize){
		return false;
	}
	ft->base.data = data;
	ft->base.size = size;
	ft->new_size = new_size;
	ft->resize = resize;
	ft->del = del;
	ft->copy = copy;
	ft->cmp = cmp;
	ft->swap = swap;
	return 1;
}


uint64_t cr8r_default_new_size(cr8r_base_ft *base, uint64_t cap){
	return cap ? cap << 1 : 8;
}

void *cr8r_default_resize(cr8r_base_ft *base, void *p, uint64_t cap){
	if(!cap){
		if(p){
			free(p);
		}
		return NULL;
	}
	return p ? realloc(p, cap*base->size) : malloc(cap*base->size);
}

int cr8r_default_cmp(const cr8r_base_ft *base, const void *a, const void *b){
	return memcmp(a, b, base->size);
}

void cr8r_default_swap(cr8r_base_ft *base, void *a, void *b){
	if(a != b){
		char buf[base->size];
		memcpy(buf, a, base->size);
		memcpy(a, b, base->size);
		memcpy(b, buf, base->size);
	}
}

bool cr8r_vec_init(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t cap){
	void *tmp = ft->resize(&ft->base, NULL, cap);
	if(!tmp){
		return false;
	}
	*self = (cr8r_vec){.buf=tmp, .cap=cap};
	return true;
}

void cr8r_vec_delete(cr8r_vec *self, cr8r_vec_ft *ft){
	if(ft->del){
		for(void *e = self->buf; e < self->buf + self->len*ft->base.size; e += ft->base.size){
			ft->del(&ft->base, e);
		}
	}
	ft->resize(&ft->base, self->buf, 0);
	*self = (cr8r_vec){};
}

bool cr8r_vec_copy(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft *ft){
	if(!cr8r_vec_init(dest, ft, src->len)){
		return false;
	}
	return cr8r_vec_augment(dest, src, ft);
}

inline static bool _sub_copy(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft *ft, uint64_t a, uint64_t b){
	if(!ft->copy){
		memcpy(dest->buf, src->buf + a*ft->base.size, (b - a)*ft->base.size);
	}else for(uint64_t i = 0; i < b - a; ++i){
		ft->copy(&ft->base, dest->buf + i*ft->base.size, src->buf + (a + i)*ft->base.size);
	}
	dest->len = b - a;
	return true;
}

bool cr8r_vec_sub(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft *ft, uint64_t a, uint64_t b){
	if(b >= src->len || b < a || !cr8r_vec_init(dest, ft, b - a)){
		return false;
	}
	return _sub_copy(dest, src, ft, a, b);
}

bool cr8r_vec_resize(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t cap){
	if(cap < self->len){
		return false;
	}
	void *tmp = ft->resize(&ft->base, self->buf, cap);
	if(!tmp){
		return false;
	}
	self->buf = tmp;
	self->cap = cap;
	return true;
}

bool cr8r_vec_trim(cr8r_vec *self, cr8r_vec_ft *ft){
	void *tmp = ft->resize(&ft->base, self->buf, self->len);
	if(!tmp){
		return false;
	}
	self->buf = tmp;
	self->cap = self->len;
	return true;
}

void cr8r_vec_clear(cr8r_vec *self, cr8r_vec_ft *ft){
	if(ft->del){
		for(void *e = self->buf; e < self->buf + self->len*ft->base.size; e += ft->base.size){
			ft->del(&ft->base, e);
		}
	}
	self->len = 0;
}

bool cr8r_vec_combine(cr8r_vec *dest, const cr8r_vec *src_a, const cr8r_vec *src_b, cr8r_vec_ft *ft){
	if(!cr8r_vec_resize(dest, ft, src_a->len + src_b->len)){
		return false;
	}
	if(!ft->copy){
		memcpy(dest->buf, src_a->buf, src_a->len*ft->base.size);
		memcpy(dest->buf + src_a->len*ft->base.size, src_b->buf, src_b->len*ft->base.size);
	}else{
		for(uint64_t i = 0; i < src_a->len; ++i){
			ft->copy(&ft->base, dest->buf + i*ft->base.size, src_a->buf + i*ft->base.size);
		}
		for(uint64_t i = 0; i < src_b->len; ++i){
			ft->copy(&ft->base, dest->buf + (src_a->len + i)*ft->base.size, src_b->buf + i*ft->base.size);
		}
	}
	dest->len = src_a->len + src_b->len;
	return true;
}

bool cr8r_vec_augment(cr8r_vec *self, const cr8r_vec *other, cr8r_vec_ft *ft){
	if(self->len + other->len > self->cap){
		if(!cr8r_vec_resize(self, ft, self->len + other->len)){
			return false;
		}
	}
	if(!ft->copy){
		memcpy(self->buf + self->len*ft->base.size, other->buf, other->len*ft->base.size);
	}else for(uint64_t i = 0; i < other->len; ++i){
		ft->copy(&ft->base, self->buf + (self->len + i)*ft->base.size, other->buf + i*ft->base.size);
	}
	self->len += other->len;
	return true;
}

void *cr8r_vec_get(cr8r_vec *self, const cr8r_vec_ft *ft, uint64_t i){
	if(self->len <= i){
		return NULL;
	}
	return self->buf + i*ft->base.size;
}

void *cr8r_vec_getx(cr8r_vec *self, const cr8r_vec_ft *ft, int64_t i){
	if(i < -(int64_t)self->len || (int64_t)self->len <= i){
		return NULL;
	}else if(i < 0){
		return self->buf + (self->len + i)*ft->base.size;
	}else{
		return self->buf + i*ft->base.size;
	}
}

uint64_t cr8r_vec_len(cr8r_vec *self){
	return self->len;
}

uint64_t cr8r_rand_uniform_u64(uint64_t a, uint64_t b){
	uint64_t l = b - a, r = 0, bytes = (71 - __builtin_clzll(l))/8;
	uint64_t ub;
	if(bytes == 8){
		ub = 0x7FFFFFFFFFFFFFFFull%l*-2;
	}else{
		ub = 1ull << (bytes*8);
		ub -= ub%l;
	}
	do{
		getrandom(&r, bytes, 0);
	}while(r >= ub);
	return r%l + a;
}

void cr8r_vec_shuffle(cr8r_vec *self, cr8r_vec_ft *ft){
	for(uint64_t i = self->len - 1, j; i; --i){
		j = cr8r_rand_uniform_u64(0, i + 1);
		ft->swap(&ft->base, self->buf + i*ft->base.size, self->buf + j*ft->base.size);
	}
}

bool cr8r_vec_pushr(cr8r_vec *self, cr8r_vec_ft *ft, const void *e){
	if(self->len == self->cap){
		uint64_t new_cap = ft->new_size(&ft->base, self->cap);
		void *tmp = ft->resize(&ft->base, self->buf, new_cap);
		if(!tmp){
			return false;
		}
		self->buf = tmp;
		self->cap = new_cap;
	}
	memcpy(self->buf + self->len++*ft->base.size, e, ft->base.size);
	return true;
}

bool cr8r_vec_popr(cr8r_vec *self, cr8r_vec_ft *ft, void *o){
	if(!self->len){
		return false;
	}
	memcpy(o, self->buf + (--self->len)*ft->base.size, ft->base.size);
	return true;
}

//Accessing the left end of a vector is O(n) -- slow.
bool cr8r_vec_pushl(cr8r_vec *self, cr8r_vec_ft *ft, const void *e){
	if(self->len == self->cap){
		uint64_t new_cap = ft->new_size(&ft->base, self->cap);
		void *tmp = ft->resize(&ft->base, self->buf, new_cap);
		if(!tmp){
			return false;
		}
		self->buf = tmp;
		self->cap = new_cap;
	}
	memmove(self->buf + ft->base.size, self->buf, self->len++*ft->base.size);
	memcpy(self->buf, e, ft->base.size);
	return true;
}

bool cr8r_vec_popl(cr8r_vec *self, cr8r_vec_ft *ft, void *o){
	if(!self->len){
		return false;
	}
	memcpy(o, self->buf, ft->base.size);
	memmove(self->buf, self->buf + ft->base.size, (--self->len)*ft->base.size);
	return true;
}

bool cr8r_vec_filter(cr8r_vec *self, cr8r_vec_ft *ft, bool (*pred)(const void*)){
	uint64_t j = 0;
	for(uint64_t i = 0; i < self->len; ++i){
		if(pred(self->buf + i*ft->base.size)){
			if(i > j){
				memcpy(self->buf + j*ft->base.size, self->buf + i*ft->base.size, ft->base.size);
			}
			++j;
		}else if(ft->del){
			ft->del(&ft->base, self->buf + i*ft->base.size);
		}
	}
	self->len = j;
	return cr8r_vec_trim(self, ft);
}

bool cr8r_vec_filtered(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft *ft, bool (*pred)(const void*)){
	dest->len = 0;
	if(!cr8r_vec_resize(dest, ft, src->len)){
		return false;
	}
	if(!ft->copy){
		for(void *e = src->buf; e < src->buf + src->len*ft->base.size; e += ft->base.size){
			if(pred(e)){
				cr8r_vec_pushr(dest, ft, e);
			}
		}
	}else{
		for(void *e = src->buf; e < src->buf + src->len*ft->base.size; e += ft->base.size){
			if(pred(e)){
				ft->copy(&ft->base, dest->buf + (dest->len++)*ft->base.size, e);
			}
		}
	}
	cr8r_vec_trim(dest, ft);
	return true;
}

bool cr8r_vec_map(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft *src_ft, cr8r_vec_ft *dest_ft, void (*f)(void *o, const void *e)){
	dest->len = 0;
	if(!cr8r_vec_resize(dest, dest_ft, src->len)){
		return false;
	}
	for(uint64_t i = 0; i < src->len; ++i){
		f(dest->buf + i*dest_ft->base.size, src->buf + i*src_ft->base.size);
	}
	dest->len = src->len;
	return true;
}

int cr8r_vec_forEachPermutation(cr8r_vec *self, cr8r_vec_ft *ft, void (*f)(const cr8r_vec *self, const cr8r_vec_ft *ft)){
	uint64_t *c = calloc(self->len, sizeof(uint64_t));
	if(!c){
		return 0;
	}
	f(self, ft);
	for(uint64_t i = 0; i < self->len; ++i){
		if(c[i] < i){
			ft->swap(&ft->base, self->buf + (i&1 ? c[i] : 0)*ft->base.size, self->buf + i*ft->base.size);
			f(self, ft);
			c[i]++;
			i = 0;
		}else{
			c[i] = 0;
		}
	}
	free(c);
	return 1;
}

bool cr8r_vec_all(const cr8r_vec *self, const cr8r_vec_ft *ft, bool (*pred)(const void*)){
	for(void *e = self->buf; e < self->buf + self->len*ft->base.size; e += ft->base.size){
		if(!pred(e)){
			return false;
		}
	}
	return true;
}

bool cr8r_vec_any(const cr8r_vec *self, const cr8r_vec_ft *ft, bool (*pred)(const void*)){
	for(void *e = self->buf; e < self->buf + self->len*ft->base.size; e += ft->base.size){
		if(pred(e)){
			return true;
		}
	}
	return false;
}

bool cr8r_vec_contains(const cr8r_vec *self, const cr8r_vec_ft *ft, const void *e){
	return cr8r_vec_index(self, ft, e) != -1;
}

int64_t cr8r_vec_index(const cr8r_vec *self, const cr8r_vec_ft *ft, const void *e){
	for(uint64_t i = 0; i < self->len; ++i){
		if(!ft->cmp(&ft->base, self->buf + i*ft->base.size, e)){
			return i;
		}
	}
	return -1;
}

void cr8r_vec_reverse(cr8r_vec *self, cr8r_vec_ft *ft){
	if(self->len < 2){
		return;
	}
	char buf[ft->base.size];
	for(uint64_t i = 0, j = self->len - 1; i < j; ++i, --j){
		memcpy(buf, self->buf + i*ft->base.size, ft->base.size);
		memcpy(self->buf + i*ft->base.size, self->buf + j*ft->base.size, ft->base.size);
		memcpy(self->buf + j*ft->base.size, buf, ft->base.size);
	}
}

bool cr8r_vec_reversed(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft *ft){
	if(!cr8r_vec_init(dest, ft, src->len)){
		return false;
	}
	if(ft->copy){
		for(uint64_t i = 0, j = src->len - 1; i < src->len; ++i, --j){
			ft->copy(&ft->base, dest->buf + i*ft->base.size, src->buf + j*ft->base.size);
		}
	}else for(uint64_t i = 0, j = src->len - 1; i < src->len; ++i, --i){
		memcpy(dest->buf + i*ft->base.size, src->buf + j*ft->base.size, ft->base.size);
	}
	return true;
}

void *cr8r_vec_foldr(const cr8r_vec *self, const cr8r_vec_ft *ft, void *init, void *(*f)(void *acc, const void *e)){
	for(void *e = self->buf; e < self->buf + self->len*ft->base.size; e += ft->base.size){
		init = f(init, e);
	}
	return init;
}

void *cr8r_vec_foldl(const cr8r_vec *self, const cr8r_vec_ft *ft, void *init, void *(*f)(void *acc, const void *e)){
	for(void *e = self->buf + (self->len - 1)*ft->base.size; e >= self->buf; e -= ft->base.size){
		init = f(init, e);
	}
	return init;
}

void cr8r_vec_sort(cr8r_vec *self, cr8r_vec_ft *ft){
	cr8r_heap_ify(self, ft, 1);
	uint64_t len = self->len;
	char buf[ft->base.size];
	while(self->len > 1){
		cr8r_heap_pop(self, ft, buf, 1);
		memcpy(self->buf + self->len*ft->base.size, buf, ft->base.size);
	}
	self->len = len;
}

bool cr8r_vec_sorted(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft *ft){
	if(!cr8r_vec_copy(dest, src, ft)){
		return false;
	}
	cr8r_vec_sort(dest, ft);
	return true;
}

bool cr8r_vec_containss(const cr8r_vec *self, const cr8r_vec_ft *ft, const void *e){
	return cr8r_vec_indexs(self, ft, e) != -1;
}

int64_t cr8r_vec_indexs(const cr8r_vec *self, const cr8r_vec_ft *ft, const void *e){
	uint64_t a = 0, b = self->len;
	while(a < b){
		uint64_t i = (a + b) >> 1;
		int ord = ft->cmp(&ft->base, e, self->buf + i*ft->base.size);
		if(!ord){
			return i;
		}else if(ord < 0){
			b = i;
		}else if(ord > 0){
			a = i + 1;
		}
	}
	return -1;
}

int cr8r_vec_cmp(const cr8r_vec *a, const cr8r_vec *b, const cr8r_vec_ft *ft){
	int ord;
	for(uint64_t i = 0;; ++i){
		if(i >= b->len){
			return i < a->len;
		}else if(i >= a->len){
			return -1;
		}
		ord = ft->cmp(&ft->base, a->buf + i*ft->base.size, b->buf + i*ft->base.size);
		if(ord){
			return ord;
		}
	}
}

cr8r_vec_ft cr8r_vecft_u64 = {
	.base = {
		.data = NULL,
		.size = sizeof(uint64_t)
	},
	.new_size = cr8r_default_new_size,
	.resize = cr8r_default_resize,
	.del = NULL,
	.copy = NULL,
	.cmp = cr8r_default_cmp_u64,
	.swap = cr8r_default_swap
};

