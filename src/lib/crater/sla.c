#include <stdlib.h>

#include <crater/sla.h>

bool cr8r_sla_init(cr8r_sla *self, uint64_t elem_size, uint64_t cap){
	if(!cap || elem_size < sizeof(void*)){
		return 0;
	}
	self->elem_size = elem_size;
	char (*slab)[elem_size] = malloc(cap*elem_size);//start the allocator with one block of length cap
	if(!slab){
		return 0;
	}else if(!(self->slabs = malloc(1*sizeof(void*)))){//put slab in an array
		free(slab);
		return 0;
	}
	self->slab_cap = cap;
	self->first_elem = self->slabs[0] = slab;
	self->slabs_len = 1;
	for(uint64_t i = 0; i + 1 < cap; ++i){//fill the stack with the pointers
		*(void**)(slab + i) = slab + i + 1;
	}
	*(void**)(slab + cap - 1) = NULL;
	return 1;
}

void cr8r_sla_delete(cr8r_sla *self){
	for(uint64_t i = 0; i < self->slabs_len; ++i){
		free(self->slabs[i]);
	}
	free(self->slabs);
	*self = (cr8r_sla){};
}

void *cr8r_sla_alloc(cr8r_sla *self){
	if(!self->first_elem){
		uint64_t slab_cap = self->slab_cap << 1;
		char (*slab)[self->elem_size] = malloc(slab_cap*self->elem_size);
		void **slabs;
		if(!slab){
			return NULL;
		}else if(!(slabs = realloc(self->slabs, (self->slabs_len + 1)*sizeof(void*)))){
			free(slab);
			return NULL;
		}
		self->first_elem = slabs[self->slabs_len++] = (void*)slab;
		self->slabs = slabs;
		for(uint64_t i = 0; i + 1 < slab_cap; ++i){
			*(void**)(slab + i) = slab + i + 1;
		}
		*(void**)(slab + slab_cap - 1) = NULL;
		self->slab_cap = slab_cap;
	}
	void *ret = self->first_elem;
	self->first_elem = *(void**)self->first_elem;
	return ret;
}

void cr8r_sla_free(cr8r_sla *self, void *p){
	*(void**)p = self->first_elem;
	self->first_elem = p;
}

