#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <crater/cll.h>


bool cr8r_cll_ft_init(cr8r_cll_ft *ft,
	void *data, uint64_t size,
	void *(*alloc)(cr8r_base_ft*),
	void (*del)(cr8r_base_ft*, void*),
	void (*copy)(cr8r_base_ft*, void *dest, const void *src),
	int (*cmp)(const cr8r_base_ft*, const void*, const void*)
){
	if(!alloc || !del){
		return false;
	}
	ft->base.data = data;
	ft->base.size = size;
	ft->alloc = alloc;
	ft->del = del;
	ft->copy = copy;
	ft->cmp = cmp;
	return true;
}

bool cr8r_cll_ft_initsla(cr8r_cll_ft *ft,
	cr8r_sla *sla, uint64_t size, uint64_t reserve,
	void (*copy)(cr8r_base_ft*, void*, const void*),
	int (*cmp)(const cr8r_base_ft*, const void*, const void*)
){
	if(!cmp || !sla || !cr8r_sla_init(sla, offsetof(cr8r_cll_node, data) + size, reserve)){
		return false;
	}
	ft->base.data = sla;
	ft->base.size = size;
	ft->alloc = cr8r_default_alloc_sla;
	ft->del = cr8r_default_free_sla;
	ft->copy = copy;
	ft->cmp = cmp;
	return true;
}


cr8r_cll_node *cr8r_cll_new(cr8r_cll_ft *ft, const void *data){
	cr8r_cll_node *ret = ft->alloc(&ft->base);
	if(ret){
		ret->next = ret;
		memcpy(ret->data, data, ft->base.size);
	}
	return ret;
}

void cr8r_cll_delete(cr8r_cll_node *self, cr8r_cll_ft *ft){
	if(!self){
		return;
	}
	for(cr8r_cll_node *it = self, *next = it->next;; next = it->next){
		ft->del(&ft->base, it);
		if((it = next) == self){
			break;
		}
	}
}

cr8r_cll_node *cr8r_cll_copy(const cr8r_cll_node *self, cr8r_cll_ft *ft){
	if(!self){
		return NULL;
	}
	cr8r_cll_node *ret = ft->alloc(&ft->base), *ot = ret;
	if(!ret){
		return NULL;
	}
	if(ft->copy){
		ft->copy(&ft->base, ret->data, self->data);
	}else{
		memcpy(ret->data, self->data, ft->base.size);
	}
	for(cr8r_cll_node *it = self->next; it != self; it = it->next, ot = ot->next){
		if(!(ot->next = ft->alloc(&ft->base))){
			cr8r_cll_delete(ot->next = ret, ft);
			return NULL;
		}
		if(ft->copy){
			ft->copy(&ft->base, ot->next->data, it->data);
		}else{
			memcpy(ot->next->data, it->data, ft->base.size);
		}
	}
	return ot->next = ret;
}

cr8r_cll_node *cr8r_cll_from(cr8r_cll_ft *ft, uint64_t len, const void *a){
	if(!len){
		return NULL;
	}
	cr8r_cll_node *h = cr8r_cll_new(ft, a), *ot = h;
	if(!h){
		return NULL;
	}
	for(uint64_t i = 1; i < len; ++i, ot = ot->next){
		if(!(ot->next = ft->alloc(&ft->base))){
			cr8r_cll_delete(ot->next = h, ft);
			return NULL;
		}
		if(ft->copy){
			ft->copy(&ft->base, ot->next->data, a + i*ft->base.size);
		}else{
			memcpy(ot->next->data, a + i*ft->base.size, ft->base.size);
		}
	}
	ot->next = h;
	return ot;
}

bool cr8r_cll_pushl(cr8r_cll_node **self, cr8r_cll_ft *ft, const void *val){
	cr8r_cll_node *n = cr8r_cll_new(ft, val);
	if(!n){
		return false;
	}
	*self = cr8r_cll_combine(n, *self, ft);
	return true;
}

bool cr8r_cll_popl(cr8r_cll_node **self, cr8r_cll_ft *ft, void *out){
	if(!*self){
		return false;
	}
	cr8r_cll_node *h = (*self)->next;
	memcpy(out, h->data, ft->base.size);
	if(h == *self){
		*self = NULL;
	}else{
		(*self)->next = h->next;
	}
	ft->del(&ft->base, h);
	return true;
}

bool cr8r_cll_pushr(cr8r_cll_node **self, cr8r_cll_ft *ft, const void *val){
	cr8r_cll_node *n = cr8r_cll_new(ft, val);
	if(!n){
		return false;
	}
	*self = cr8r_cll_combine(*self, n, ft);
	return true;
}

//warning: this is an O(n) operation
bool cr8r_cll_popr(cr8r_cll_node **self, cr8r_cll_ft *ft, void *out){
	if(!*self){
		return false;
	}
	memcpy(out, (*self)->data, ft->base.size);
	cr8r_cll_node *it = (*self)->next;
	if(it == *self){
		*self = NULL;
	}else for(; it->next != *self; it = it->next);
	it->next = (*self)->next;
	ft->del(&ft->base, *self);
	*self = it;
	return true;
}

bool cr8r_cll_filtered(cr8r_cll_node **dest, const cr8r_cll_node *src, cr8r_cll_ft *ft, bool (*pred)(const void*)){
	if(!src){
		*dest = NULL;
		return false;
	}
	cr8r_cll_node *ret = ft->alloc(&ft->base), *ot = ret;
	if(!ret){
		return false;
	}
	if(ft->copy){
		ft->copy(&ft->base, ret->data, src->data);
	}else{
		memcpy(ret->data, src->data, ft->base.size);
	}
	for(cr8r_cll_node *it = src->next; it != src; it = it->next){
		if(!pred(it->data)){
			continue;
		}
		if(!(ot->next = ft->alloc(&ft->base))){
			cr8r_cll_delete(ot->next = ret, ft);
			return false;
		}
		ot = ot->next;
		if(ft->copy){
			ft->copy(&ft->base, ot->data, it->data);
		}else{
			memcpy(ot->data, it->data, ft->base.size);
		}
	}
	*dest = ot->next = ret;
	return true;
}

void cr8r_cll_filter(cr8r_cll_node **self, cr8r_cll_ft *ft, bool (*pred)(const void*)){
	if(!*self){
		return;
	}
	cr8r_cll_node h = {}, *ot = &h, *it = (*self)->next;
	(*self)->next = NULL;
	for(cr8r_cll_node *next; it; it = next){
		next = it->next;
		if(pred(it->data)){
			ot = ot->next = it;
		}else{
			ft->del(&ft->base, it);
		}
	}
	ot->next = h.next;
	*self = ot;
}

bool cr8r_cll_mapped(cr8r_cll_node **dest, const cr8r_cll_node *src, cr8r_cll_ft *dest_ft, void (*f)(void *o, const void *e)){
	if(!src){
		*dest = NULL;
		return true;
	}
	cr8r_cll_node *ret = dest_ft->alloc(&dest_ft->base), *ot = ret;
	if(!ret){
		return false;
	}
	f(ret->data, src->data);
	for(cr8r_cll_node *it = src->next; it != src; it = it->next, ot = ot->next){
		if(!(ot->next = dest_ft->alloc(&dest_ft->base))){
			cr8r_cll_delete(ot->next = ret, dest_ft);
			return false;
		}
		f(ot->next->data, it->data);
	}
	*dest = ot->next = ret;
	return true;
}

void cr8r_cll_forEach(cr8r_cll_node *self, cr8r_cll_ft *ft, void (*f)(void*)){
	if(!self){
		return;
	}
	for(cr8r_cll_node *it = self->next;;){
		f(it->data);
		if((it = it->next) == self->next){
			break;
		}
	}
}

cr8r_cll_node *cr8r_cll_combine(cr8r_cll_node *a, cr8r_cll_node *b, cr8r_cll_ft *ft){
	if(!a){
		return b;
	}else if(!b){
		return a;
	}
	cr8r_cll_node *h = a->next;
	a->next = b->next;
	b->next = h;
	return b;
}

bool cr8r_cll_all(const cr8r_cll_node *self, cr8r_cll_ft *ft, bool (*pred)(const void*)){
	if(!self){
		return true;
	}
	for(const cr8r_cll_node *it = self;;){
		if(!pred(it->data)){
			return false;
		}
		if((it = it->next) == self){
			return true;
		}
	}
}

bool cr8r_cll_any(const cr8r_cll_node *self, cr8r_cll_ft *ft, bool (*pred)(const void*)){
	if(!self){
		return false;
	}
	for(const cr8r_cll_node *it = self;;){
		if(pred(it->data)){
			return true;
		}
		if((it = it->next) == self){
			return false;
		}
	}
}

cr8r_cll_node *cr8r_cll_lsearch(cr8r_cll_node *self, cr8r_cll_ft *ft, const void *e){
	if(!self){
		return NULL;
	}
	for(cr8r_cll_node *it = self->next;;){
		if(ft->cmp(&ft->base, e, it->data)){
			return it;
		}
		if((it = it->next) == self->next){
			return NULL;
		}
	}
}

void *cr8r_cll_foldr(const cr8r_cll_node *self, cr8r_cll_ft *ft, void *init, void *(*f)(void *acc, const void *e)){
	if(!self){
		return NULL;
	}
	for(const cr8r_cll_node *it = self->next;; it = it->next){
		init = f(init, it->data);
		if(it == self){
			return init;
		}
	}
}

//look at merge_exponential in my pheap code elsewhere
void cr8r_cll_sort(cr8r_cll_node **self, cr8r_cll_ft *ft);
cr8r_cll_node *cr8r_cll_sorted(const cr8r_cll_node *self, cr8r_cll_ft *ft);

void cr8r_cll_reverse(cr8r_cll_node **self, cr8r_cll_ft *ft){
	cr8r_cll_node *r = *self;
	if(!r){
		return;
	}
	cr8r_cll_node *h = r->next;
	if(h == r){
		return;
	}
	r->next = NULL;
	for(cr8r_cll_node *it = h, *next, *ot = NULL; it; it = next){
		next = it->next;
		it->next = ot;
		ot = it;
	}
	h->next = r;
	*self = h;
}

cr8r_cll_node *cr8r_cll_reversed(const cr8r_cll_node *self, cr8r_cll_ft *ft){
	if(!self){
		return NULL;
	}
	cr8r_cll_node *it = self->next;
	cr8r_cll_node *r = ft->alloc(&ft->base);
	if(!r){
		return NULL;
	}
	ft->copy(&ft->base, r->data, it->data);
	cr8r_cll_node *ot = r, *n;
	for(it = it->next; it != self->next; it = it->next){
		if(!(n = ft->alloc(&ft->base))){
			cr8r_cll_delete(r->next = ot, ft);
			return NULL;
		}
		ft->copy(&ft->base, n->data, it->data);
		n->next = ot;
		ot = n;
	}
	r->next = ot;
	return r;
}

