#include <stdlib.h>
#include <string.h>

#include <crater/vec.h>
#include <crater/heap.h>

#ifdef DEBUG
#include <crater/vec_check.h>
#endif


bool cr8r_vec_ft_init(cr8r_vec_ft *ft,
	void *data, uint64_t size,
	uint64_t (*new_size)(cr8r_base_ft*, uint64_t cap),
	void *(*resize)(cr8r_base_ft*, void *p, uint64_t cap),
	void (*del)(cr8r_base_ft*, void *p),
	void (*copy)(cr8r_base_ft*, void *dest, const void *src),
	int (*cmp)(const cr8r_base_ft*, const void *a, const void *b),
	void (*swap)(cr8r_base_ft*, void *a, void *b)
){
	ft->base.data = data;
	ft->base.size = size;
	ft->new_size = new_size ?: cr8r_default_new_size;
	ft->resize = resize ?: cr8r_default_resize;
	ft->del = del;
	ft->copy = copy;
	ft->cmp = cmp;
	ft->swap = swap ?: cr8r_default_swap;
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

void *cr8r_default_resize_pass(cr8r_base_ft *base, void *p, uint64_t cap){
	return NULL;
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
		return !(cap && ft->base.size);
	}
	*self = (cr8r_vec){.buf=tmp, .cap=cap};
	return 1;
}

void cr8r_vec_delete(cr8r_vec *self, cr8r_vec_ft *ft){
	cr8r_vec_clear(self, ft);
	ft->resize(&ft->base, self->buf, 0);
	*self = (cr8r_vec){};
}

bool cr8r_vec_copy(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft *ft){
	if(!cr8r_vec_init(dest, ft, src->len)){
		return 0;
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
	return 1;
}

bool cr8r_vec_sub(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft *ft, uint64_t a, uint64_t b){
	if(b > src->len || b < a || !cr8r_vec_init(dest, ft, b - a)){
		return 0;
	}
	return _sub_copy(dest, src, ft, a, b);
}

bool cr8r_vec_resize(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t cap){
	if(cap < self->len){
		return 0;
	}
	void *tmp = ft->resize(&ft->base, self->buf, cap);
	if(!tmp){
		return !(cap && ft->base.size);
	}
	self->buf = tmp;
	self->cap = cap;
	return 1;
}

bool cr8r_vec_trim(cr8r_vec *self, cr8r_vec_ft *ft){
	void *tmp = ft->resize(&ft->base, self->buf, self->len);
	if(!tmp){
		if(!(self->len && ft->base.size)){
			self->buf = NULL;
			self->cap = self->len;
			return 1;
		}
		return 0;
	}
	self->buf = tmp;
	self->cap = self->len;
	return 1;
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
	if(!cr8r_vec_init(dest, ft, src_a->len + src_b->len)){
		return 0;
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
	return 1;
}

bool cr8r_vec_augment(cr8r_vec *self, const cr8r_vec *other, cr8r_vec_ft *ft){
	if(self->len + other->len > self->cap){
		uint64_t cap = ft->new_size(&ft->base, self->cap);
		if(self->len + other->len > cap){
			cap = self->len + other->len;
		}
		if(!cr8r_vec_resize(self, ft, cap)){
			return 0;
		}
	}
	if(!ft->copy){
		memcpy(self->buf + self->len*ft->base.size, other->buf, other->len*ft->base.size);
	}else for(uint64_t i = 0; i < other->len; ++i){
		ft->copy(&ft->base, self->buf + (self->len + i)*ft->base.size, other->buf + i*ft->base.size);
	}
	self->len += other->len;
	return 1;
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

void cr8r_vec_shuffle(cr8r_vec *self, cr8r_vec_ft *ft, cr8r_prng *prng){
	for(uint64_t i = self->len - 1, j; i; --i){
		j = cr8r_prng_uniform_u64(prng, 0, i + 1);
		ft->swap(&ft->base, self->buf + i*ft->base.size, self->buf + j*ft->base.size);
	}
}

static inline bool ensure_cap(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t cap){
	if(cap > self->cap && ft->base.size){
		uint64_t new_cap = ft->new_size(&ft->base, self->cap);
		if(cap > new_cap){
			new_cap = cap;
		}
		void *tmp = ft->resize(&ft->base, self->buf, new_cap);
		if(!tmp){
			return !ft->base.size;
		}
		self->buf = tmp;
		self->cap = new_cap;
	}
	return 1;
}

bool cr8r_vec_pushr(cr8r_vec *self, cr8r_vec_ft *ft, const void *e){
	if(!ensure_cap(self, ft, self->len + 1)){
		return 0;
	}else if(ft->base.size){
		memcpy(self->buf + self->len++*ft->base.size, e, ft->base.size);
	}
	return 1;
}

bool cr8r_vec_popr(cr8r_vec *self, cr8r_vec_ft *ft, void *o){
	if(!self->len){
		return 0;
	}
	memcpy(o, self->buf + (--self->len)*ft->base.size, ft->base.size);
	return 1;
}

//Accessing the left end of a vector is O(n) -- slow.
bool cr8r_vec_pushl(cr8r_vec *self, cr8r_vec_ft *ft, const void *e){
	if(!ensure_cap(self, ft, self->len + 1)){
		return 0;
	}else if(ft->base.size){
		memmove(self->buf + ft->base.size, self->buf, self->len++*ft->base.size);
		memcpy(self->buf, e, ft->base.size);
	}
	return 1;
}

bool cr8r_vec_popl(cr8r_vec *self, cr8r_vec_ft *ft, void *o){
	if(!self->len){
		return 0;
	}
	memcpy(o, self->buf, ft->base.size);
	memmove(self->buf, self->buf + ft->base.size, (--self->len)*ft->base.size);
	return 1;
}

bool cr8r_vec_filter(cr8r_vec *self, cr8r_vec_ft *ft, cr8r_vec_pred pred, void *data){
	uint64_t j = 0;
	if(ft->swap == cr8r_default_swap){
		for(uint64_t i = 0; i < self->len; ++i){
			if(pred(ft, self->buf + i*ft->base.size, data)){
				if(i > j){
					memcpy(self->buf + j*ft->base.size, self->buf + i*ft->base.size, ft->base.size);
				}
				++j;
			}else if(ft->del){
				ft->del(&ft->base, self->buf + i*ft->base.size);
			}
		}
	}else{// otherwise swap is nontrivial
		// we can only swap constructed elements, so we can only delete elements at the end.
		// we can maintain two regions in the array: a "passed" region and a "failed" region.
		// the array is always split into the passed region, followed by the failed region,
		// followed by the unchecked region.  initially, we have checked no elements, so the
		// passed and failed regions both have length zero.  if the first unexplored element passes
		// the predicate, we swap it with the first failed element if there is one (otherwise we leave it)
		// and then update the passed and failed regions.  if the first unexplored element
		// fails the predicate, we extend the failed region to include it.  when there are no
		// unexplored elements left, we can delete all failed elements.
		//
		// j is the number of passed elements, so let i be the number of failed elements
		for(uint64_t i = 0; j + i < self->len;){
			if(pred(ft, self->buf + (j + i)*ft->base.size, data)){
				if(i){
					ft->swap(&ft->base, self->buf + j*ft->base.size, self->buf + (j + i)*ft->base.size);
				}
				++j;
			}else{
				++i;
			}
		}
		if(ft->del){
			for(uint64_t i = j; i < self->len; ++i){
				ft->del(&ft->base, self->buf + i*ft->base.size);
			}
		}
	}
	self->len = j;
	return cr8r_vec_trim(self, ft);
}

bool cr8r_vec_filtered(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft *ft, cr8r_vec_pred pred, void *data){
	if(!cr8r_vec_init(dest, ft, src->len)){
		return 0;
	}
	if(!ft->copy){
		for(void *e = src->buf; e < src->buf + src->len*ft->base.size; e += ft->base.size){
			if(pred(ft, e, data)){
				cr8r_vec_pushr(dest, ft, e);
			}
		}
	}else{
		for(void *e = src->buf; e < src->buf + src->len*ft->base.size; e += ft->base.size){
			if(pred(ft, e, data)){
				ft->copy(&ft->base, dest->buf + (dest->len++)*ft->base.size, e);
			}
		}
	}
	cr8r_vec_trim(dest, ft);
	return 1;
}

bool cr8r_vec_map(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft *src_ft, cr8r_vec_ft *dest_ft, cr8r_vec_mapper f, void *data){
	if(!cr8r_vec_init(dest, dest_ft, src->len)){
		return 0;
	}
	for(uint64_t i = 0; i < src->len; ++i){
		f(src_ft, dest_ft, dest->buf + i*dest_ft->base.size, src->buf + i*src_ft->base.size, data);
	}
	dest->len = src->len;
	return 1;
}

int cr8r_vec_forEachPermutation(cr8r_vec *self, cr8r_vec_ft *ft, void (*f)(const cr8r_vec *self, const cr8r_vec_ft *ft, void *data), void *data){
	uint64_t *c = calloc(self->len, sizeof(uint64_t));
	if(!c){
		return 0;
	}
	f(self, ft, data);
	for(uint64_t i = 0; i < self->len; ++i){
		if(c[i] < i){
			ft->swap(&ft->base, self->buf + (i&1 ? c[i] : 0)*ft->base.size, self->buf + i*ft->base.size);
			f(self, ft, data);
			c[i]++;
			i = 0;
		}else{
			c[i] = 0;
		}
	}
	free(c);
	return 1;
}

bool cr8r_vec_all(const cr8r_vec *self, const cr8r_vec_ft *ft, cr8r_vec_pred pred, void *data){
	for(void *e = self->buf; e < self->buf + self->len*ft->base.size; e += ft->base.size){
		if(!pred(ft, e, data)){
			return 0;
		}
	}
	return 1;
}

bool cr8r_vec_any(const cr8r_vec *self, const cr8r_vec_ft *ft, cr8r_vec_pred pred, void *data){
	for(void *e = self->buf; e < self->buf + self->len*ft->base.size; e += ft->base.size){
		if(pred(ft, e, data)){
			return 1;
		}
	}
	return 0;
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

void *cr8r_vec_exm(cr8r_vec *self, const cr8r_vec_ft *ft, int ord){
	if(!self->len){
		return NULL;
	}
	void *exm = self->buf;
	for(uint64_t i = 1; i < self->len; ++i){
		void *e = self->buf + i*ft->base.size;
		if(ord*ft->cmp(&ft->base, e, exm) < 0){
			exm = e;
		}
	}
	return exm;
}

void cr8r_vec_reverse(cr8r_vec *self, cr8r_vec_ft *ft){
	if(self->len < 2){
		return;
	}
	for(uint64_t i = 0, j = self->len - 1; i < j; ++i, --j){
		ft->swap(&ft->base, self->buf + i*ft->base.size, self->buf + j*ft->base.size);
	}
}

bool cr8r_vec_reversed(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft *ft){
	if(!cr8r_vec_init(dest, ft, src->len)){
		return 0;
	}
	if(ft->copy){
		for(uint64_t i = 0, j = src->len - 1; i < src->len; ++i, --j){
			ft->copy(&ft->base, dest->buf + i*ft->base.size, src->buf + j*ft->base.size);
		}
	}else for(uint64_t i = 0, j = src->len - 1; i < src->len; ++i, --j){
		memcpy(dest->buf + i*ft->base.size, src->buf + j*ft->base.size, ft->base.size);
	}
	dest->len = src->len;
	return 1;
}

void *cr8r_vec_foldr(const cr8r_vec *self, const cr8r_vec_ft *ft, cr8r_vec_accumulator f, void *init){
	for(void *e = self->buf; e < self->buf + self->len*ft->base.size; e += ft->base.size){
		init = f(ft, init, e);
	}
	return init;
}

void *cr8r_vec_foldl(const cr8r_vec *self, const cr8r_vec_ft *ft, cr8r_vec_accumulator f, void *init){
	for(void *e = self->buf + (self->len - 1)*ft->base.size; e >= self->buf; e -= ft->base.size){
		init = f(ft, init, e);
	}
	return init;
}

void cr8r_vec_sort(cr8r_vec *self, cr8r_vec_ft *ft){
	// TODO: call nontrivial ft->swap
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
		return 0;
	}
	cr8r_vec_sort(dest, ft);
	return 1;
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

static inline void cr8r_vec_sort_i(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b){
	void *start = self->buf + a*ft->base.size;
	void *end = self->buf + b*ft->base.size;
	for(void *it = start + ft->base.size; it < end; it += ft->base.size){
		for(void *jt = it; jt > start && ft->cmp(&ft->base, jt - ft->base.size, jt) > 0; jt -= ft->base.size){
			ft->swap(&ft->base, jt - ft->base.size, jt);
		}
	}
}

void *cr8r_vec_pivot_mm(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b){
	if(a >= b || b > self->len){
		return NULL;
	}
	// Selecting a pivot by median of medians is mutually recursive with quickselect itself.
	// First we split the vector into chunks of 5 and find the median of each chunk.
	// As we do this, we swap the median to the front of the array so at the end
	// the first 20% or so of the array is composed of the medians of the chunks.
	// Then we find the median of this portion of the array with quickselect.
	// This is guaranteed to be >= about 3/10 of the array and <= about 3/10 as well.
	uint64_t j = a;
	for(uint64_t i = a; i < b; i += 5){
		uint64_t curr_len = b - i < 5 ? b - i : 5;
		if(curr_len < 3){
			ft->swap(&ft->base, self->buf + (j++)*ft->base.size, self->buf + i*ft->base.size);
		}else{
			cr8r_vec_sort_i(self, ft, i, i + curr_len);
			ft->swap(&ft->base, self->buf + (j++)*ft->base.size, self->buf + (i + curr_len/2)*ft->base.size);
		}
	}
	return cr8r_vec_ith(self, ft, a, a + (b - a + 4)/5, (b - a + 4)/10);
}

void *cr8r_vec_pivot_m3(const cr8r_vec *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b){
	if(a >= b || b > self->len){
		return NULL;
	}
	void *fst = self->buf + a*ft->base.size;
	void *lst = self->buf + (b - 1)*ft->base.size;
	void *mid = self->buf + (a + b - 1)/2*ft->base.size;
	int ord = ft->cmp(&ft->base, fst, mid);
	int sorted;
	if(!ord){
		return mid;
	}else if((sorted = ord*ft->cmp(&ft->base, mid, lst))){
		if(sorted > 0){
			return mid;
		}else if(ft->cmp(&ft->base, lst, fst)*ord >= 0){
			return lst;
		}else{
			return fst;
		}
	}else{
		return mid;
	}
}

void *cr8r_vec_partition(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b, void *piv){
	if(a >= b || b > self->len){
		return NULL;
	}
	void *fst = self->buf + a*ft->base.size;
	void *lst = self->buf + (b - 1)*ft->base.size;
	if(piv < fst || piv > lst){
		return NULL;
	}
	if(piv != lst){
		ft->swap(&ft->base, piv, lst);
	}
	// now, lst points to the pivot
	void *it = fst - ft->base.size;
	void *jt = lst;
	// initially, *it is one before the start of the subrange, so that after
	// adding 1 to it, all elements preceding it (ie no elements) will be
	// known to be < pivot.
	// on the other hand, *jt points to the pivot so all elements beginning
	// at *jt are trivially >= the pivot (since *jt points to the last element
	// initially and the last element holds the pivot)
	while(1){
		do{
			it += ft->base.size;
		}while(it < jt && ft->cmp(&ft->base, it, lst) < 0);
		// now, *it >= pivot but the elements preceding *it are < pivot
		if(it == jt){
			break;
		}
		do{
			// currently, *jt and subsequent elements are all >= pivot
			jt -= ft->base.size;
		}while(it < jt && ft->cmp(&ft->base, jt, lst) >= 0);
		if(it == jt){
			break;
		}
		ft->swap(&ft->base, it, jt);
	}
	// *jt and subsequent elements are >= pivot, so partitioning
	// is complete and *jt is the first element >= pivot
	if(jt != lst){
		// we need to ensure the pivot is placed at the beginning of the
		// >= pivot region, so we have a (possibly empty) < pivot region,
		// the pivot, and a (possibly empty) >= pivot region
		ft->swap(&ft->base, jt, lst);
	}
	return jt;
}

static inline void sort_end_tail(cr8r_vec_ft *ft, int ord, void *start, void *it){
	for(void *jt = it; jt > start && ord*ft->cmp(&ft->base, jt - ft->base.size, jt) > 0; jt -= ft->base.size){
		ft->swap(&ft->base, jt - ft->base.size, jt);
	}
}

// Partially sort a subrange of the given vector to find an element near the max or min.
// If i < 0, find the (b + i)th element by partially sorting the -i largest elements
// descending in the beginning of the subrange and returning a pointer to the -i largest.
// If i >= 0, find the ith element by partially sorting the i+1 smallest elements ascending
// in the beginning of the subrange and returning a pointer to the i+1 smallest.
static inline void *sort_end(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b, int64_t i){
	int ord = 1;
	if(i < 0){
		ord = -1;
		i = -i - 1;
	}
	void *start = self->buf + a*ft->base.size;
	void *res = start + i*ft->base.size;
	void *end = self->buf + b*ft->base.size;
	if(res >= end){
		return NULL;// error
	}
	// initially, [a, a + 1) = [start, start + 1) is sorted
	for(void *it = start + ft->base.size; it <= res; it += ft->base.size){
		sort_end_tail(ft, ord, start, it);
	}
	// now [a, res + 1) is sorted
	for(void *it = res + ft->base.size; it < end; it += ft->base.size){
		if(ord*ft->cmp(&ft->base, res, it) <= 0){
			// [a, res + 1) is sorted and res is <= [res + 1, it + 1)
			continue;
		}
		ft->swap(&ft->base, res, it);
		// again [a, res + 1) is sorted and res is <= [res + 1, it + 1)
		sort_end_tail(ft, ord, start, res);
	}
	return res;
}

void *cr8r_vec_ith(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b, uint64_t i){
	while(1){
		//fprintf(stderr, "cr8r_vec_ith(self, ft, %"PRIu64", %"PRIu64", %"PRIu64")\n", a, b, i);
		if(i >= b - a){
			return NULL;
		}else if(i < CR8R_VEC_ISORT_BOUND){
			return sort_end(self, ft, a, b, i);
		}else if(b - i <= CR8R_VEC_ISORT_BOUND){
			return sort_end(self, ft, a, b, (int64_t)i - (int64_t)b);
		}
		void *piv = cr8r_vec_pivot_mm(self, ft, a, b);
		piv = cr8r_vec_partition(self, ft, a, b, piv);
		if(!piv){
			return NULL;
		}
		uint64_t j = (piv - self->buf)/ft->base.size - a;
		if(i < j){
			b = a + j;
		}else if(i > j){
			i -= j + 1;
			a += j + 1;
		}else{
			return piv;
		}
	}
}

typedef struct{
	void *med;
	uint64_t med_idx; // median index
	uint64_t lb; // elements < the median occur in the range [a, lb)
	uint64_t ea;
	uint64_t eb; // elements == the median occur in the range [ea, eb)
	uint64_t ha; // elements > the median occur in the range [ha, b)
} pwm_state;

// Advance lb past any elements < med, since they belong in the range [a, lb)
// When elements == med are encountered and there is room on the left of [ea, eb),
// we swap it there and continue.
// Finally, when an element > med is encountered or lb bumps into ea, we break.
static inline void pwm_advance_le(cr8r_vec *self, cr8r_vec_ft *ft, pwm_state *st){
	while(st->lb < st->ea){
		int ord = ft->cmp(&ft->base, self->buf + st->lb*ft->base.size, st->med);
		if(ord > 0){
			break;
		}else if(ord < 0){
			++st->lb;
		}else{
			if(st->lb == --st->ea){
				break;
			}
			ft->swap(&ft->base, self->buf + st->lb*ft->base.size, self->buf + st->ea*ft->base.size);
		}
	}
}

// Advance ha (backwards) past any elements > med, since they belong in the range [ha, b)
// When elements == med are encountered and there is room on the right of [ea, eb),
// we swap in there and continue.
// Finally, when an element < med is encountered or ha bumps into eb, we break.
static inline void pwm_advance_ge(cr8r_vec *self, cr8r_vec_ft *ft, pwm_state *st){
	while(st->eb < st->ha){
		int ord = ft->cmp(&ft->base, st->med, self->buf + (st->ha - 1)*ft->base.size);
		if(ord > 0){
			break;
		}else if(ord < 0){
			--st->ha;
		}else{
			if(++st->eb == st->ha){
				break;
			}
			ft->swap(&ft->base, self->buf + (st->eb - 1)*ft->base.size, self->buf + (st->ha - 1)*ft->base.size);
		}
	}
}

// Variant of pmw_advance_le that runs when ha has bumped int eb.
// This means that lb bumping into ea is final,
// but also that if we find more elements > med we'll have to shift the == region
static inline void pwm_finish_lt(cr8r_vec *self, cr8r_vec_ft *ft, pwm_state *st){
	char tmp[ft->base.size];
	while(st->lb < st->ea){
		int ord = ft->cmp(&ft->base, self->buf + st->lb*ft->base.size, st->med);
		if(ord < 0){
			++st->lb;
		}else if(!ord){
			if(st->lb == --st->ea){
				break;
			}
			ft->swap(&ft->base, self->buf + st->lb*ft->base.size, self->buf + st->ea*ft->base.size);
		}else{ // lb is > the median so it must go on the right, so we must shift an == element over
			--st->eb;
			--st->ha;
			if(st->lb == --st->ea){
				ft->swap(&ft->base, self->buf + st->ea*ft->base.size, self->buf + st->ha*ft->base.size);
				break;
			}
			// triple swap lb, ea, and ha
			memcpy(tmp, self->buf + st->lb*ft->base.size, ft->base.size);
			memcpy(self->buf + st->lb*ft->base.size, self->buf + st->ea*ft->base.size, ft->base.size);
			memcpy(self->buf + st->ea*ft->base.size, self->buf + st->ha*ft->base.size, ft->base.size);
			memcpy(self->buf + st->ha*ft->base.size, tmp, ft->base.size);
		}
	}
}

// Variant of pmw_advance_ge that runs when lb has bumped int ea.
// This means that ha bumping into eb is final,
// but also that if we find more elements < med we'll have to shift the == region
static inline void pwm_finish_gt(cr8r_vec *self, cr8r_vec_ft *ft, pwm_state *st){
	char tmp[ft->base.size];
	while(st->eb < st->ha){
		int ord = ft->cmp(&ft->base, st->med, self->buf + (st->ha - 1)*ft->base.size);
		if(ord < 0){
			--st->ha;
		}else if(!ord){
			if(++st->eb == st->ha){
				break;
			}
			ft->swap(&ft->base, self->buf + (st->eb - 1)*ft->base.size, self->buf + (st->ha - 1)*ft->base.size);
		}else{ // ha is < the median so it must go on the left, so we must shift an == element over
			++st->lb;
			++st->ea;
			if(++st->eb == st->ha){
				ft->swap(&ft->base, self->buf + (st->ea - 1)*ft->base.size, self->buf + (st->ha - 1)*ft->base.size);
				break;
			}
			// triple swap lb - 1, ha - 1, and eb -1
			memcpy(tmp, self->buf + (st->lb - 1)*ft->base.size, ft->base.size);
			memcpy(self->buf + (st->lb - 1)*ft->base.size, self->buf + (st->ha - 1)*ft->base.size, ft->base.size);
			memcpy(self->buf + (st->ha - 1)*ft->base.size, self->buf + (st->eb - 1)*ft->base.size, ft->base.size);
			memcpy(self->buf + (st->eb - 1)*ft->base.size, tmp, ft->base.size);
		}
	}
}

void *cr8r_vec_partition_with_median(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b, void *med){
	pwm_state st = {};
	st.med = med;
	st.med_idx = (med - self->buf)/ft->base.size;
	if(a >= b || st.med_idx < a || st.med_idx >= b){
		return NULL;
	}
	// We need to partition our array but ensure the median element is placed in the center
	// of the array.  This requires a partitioning algorithm that can handle equal elements to
	// the median, since the basic partitioning algorithm could have it out of place by up to
	// as many equal elements as there are
	st.lb = a; // the elements < the median occur in the range [a, lb)
	st.ea = (b + a)/2;
	st.eb = st.ea + 1; // the elements == the median occur in the range [ea, eb)
	st.ha = b; // the elements > the median occur in the range [ha, b)
	if(st.med_idx != st.ea){
		med = self->buf + st.ea*ft->base.size;
		ft->swap(&ft->base, med, self->buf + st.med_idx*ft->base.size); // ensure the median is placed at the center
		st.med_idx = st.ea;
	}
	// now we will extend [a, lb), [ha, b), and [ea, eb) until they encompass the entire array
	// TODO: consider alternating which side we place == elements on
	while(st.lb < st.ea && st.eb < st.ha){
		// first we skip over any elements on the left < the median, and swap any == to the
		// central region
		pwm_advance_le(self, ft, &st);
		// next we skip over any elements on the right > the median, and swap any == to the
		// central region
		pwm_advance_ge(self, ft, &st);
		// now there are two possibilities: one of [a, lb) or [ha, b) bumped into [ea, eb),
		// or we have a pair of mismatched elements at lb and ha-1 which we can swap.
		if(st.lb == st.ea || st.eb == st.ha){
			break;
		}
		--st.ha;
		ft->swap(&ft->base, self->buf + st.lb*ft->base.size, self->buf + st.ha*ft->base.size);
		++st.lb;
	}
	// now either eb == ha or lb == ea, which will cause pwm_finish_lt and/or pwm_finish_gt to
	// become no-ops, respectively
	pwm_finish_lt(self, ft, &st);
	pwm_finish_gt(self, ft, &st);
	if(st.lb != st.ea || st.eb != st.ha || st.ea > st.med_idx || st.eb <= st.med_idx){
		return NULL;
	}
#ifdef DEBUG
	return cr8r_vec_check_pwm(ft, self, a, st.lb, st.ha, b, med) ? med : NULL;
#else
	return med;
#endif
}

uint64_t cr8r_powmod(uint64_t b, uint64_t e, uint64_t n){
	uint64_t res = 1;
	while(e){
		if(e&1){
			res = (unsigned __int128)res * (unsigned __int128)b % (unsigned __int128)n;
		}
		b = (unsigned __int128)b * (unsigned __int128)b % (unsigned __int128)n;
		e >>= 1;
	}
	return res;
}

uint64_t cr8r_pow_u64(uint64_t b, uint64_t e){
	uint64_t res = 1;
	while(e){
		if(e&1){
			res *= b;
		}
		b *= b;
		e >>= 1;
	}
	return res;
}

void *cr8r_default_acc_sum_u64(const cr8r_vec_ft *ft, void *_acc, const void *e){
	return (void*)((uint64_t)_acc + *(const uint64_t*)e);
}

void *cr8r_default_acc_sumpowmod_u64(const cr8r_vec_ft *ft, void *_acc, const void *e){
	uint64_t *acc = _acc;
	acc[0] = (acc[0] + cr8r_powmod(*(const uint64_t*)e, acc[1], acc[2]))%acc[2];
	return acc;
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

cr8r_vec_ft cr8r_vecft_cstr = {
	.base = {
		.data = NULL,
		.size = sizeof(char*)
	},
	.new_size = cr8r_default_new_size,
	.resize = cr8r_default_resize,
	.del = cr8r_default_free,
	.copy = cr8r_default_copy_cstr,
	.cmp = cr8r_default_cmp_cstr,
	.swap = cr8r_default_swap
};

cr8r_vec_ft cr8r_vecft_u8 = {
	.base = {
		.data = NULL,
		.size = sizeof(uint8_t)
	},
	.new_size = cr8r_default_new_size,
	.resize = cr8r_default_resize,
	.del = NULL,
	.copy = NULL,
	.cmp = cr8r_default_cmp_u8,
	.swap = cr8r_default_swap
};

