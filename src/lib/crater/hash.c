#define _POSIX_C_SOURCE  200809L
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include <crater/hash.h>

//these are the prime infimums of powers of two in the uint64_t range so which one to use can be calculated by a bit scan (__builtin_clzll() aka bsfq)
uint64_t exp_primes[64] = {0UL, 3UL, 7UL, 13UL, 31UL, 61UL, 127UL, 251UL, 509UL, 1021UL, 2039UL, 4093UL, 8191UL, 16381UL, 32749UL, 65537UL, 131071UL, 262139UL, 524287UL, 1048573UL,
	2097143UL, 4194301UL, 8388593UL, 16777213UL, 33554393UL, 67108859UL, 134217689UL, 268435399UL, 536870909UL, 1073741789UL, 2147483647UL, 4294967291UL, 8589934583UL, 17179869143UL,
	34359738337UL, 68719476731UL, 137438953447UL, 274877906899UL, 549755813881UL, 1099511627689UL, 2199023255531UL, 4398046511093UL, 8796093022151UL, 17592186044399UL,
	35184372088777UL, 70368744177643UL, 140737488355213UL, 281474976710597UL, 562949953421231UL, 1125899906842597UL, 2251799813685119UL, 4503599627370449UL, 9007199254740881UL,
	18014398509481951UL, 36028797018963913UL, 72057594037927931UL, 144115188075855859UL, 288230376151711717UL, 576460752303423433UL, 1152921504606846883UL, 2305843009213693951UL,
	4611686018427387847UL, 9223372036854775783UL, 18446744073709551557UL};


const uint64_t cr8r_hash_u64_prime = 536870909;

uint64_t cr8r_default_hash_u64(const cr8r_base_ft *ft, const void *_a){
	uint64_t a = *(const uint64_t*)_a;
	unsigned __int128 prod = a*((unsigned __int128)cr8r_hash_u64_prime);
	return (uint64_t)(prod >> 64) ^ (uint64_t)prod;
}

uint64_t cr8r_default_hash(const cr8r_base_ft *ft, const void *_a){
	const char *a = _a;
	uint64_t h = 5381;
	for(uint64_t i = 0; i < ft->size; ++i){
		h = 31*h + a[i];
	}
	return h;
}

uint64_t cr8r_default_hash_cstr(const cr8r_base_ft *ft, const void *_a){
	const char *a = *(const char**)_a;
	uint64_t h = 5381;
	for(char c = *a; c; c = *++a){
		h = 31*h + c;
	}
	return h;
}

int cr8r_default_cmp_u64(const cr8r_base_ft *ft, const void *_a, const void *_b){
	uint64_t a = *(const uint64_t*)_a, b = *(const uint64_t*)_b;
	if(a < b){
		return -1;
	}else if(a == b){
		return 0;
	}
	return 1;
}

int cr8r_default_cmp_i64(const cr8r_base_ft *ft, const void *_a, const void *_b){
	int64_t a = *(const int64_t*)_a, b = *(const int64_t*)_b;
	if(a < b){
		return -1;
	}else if(a == b){
		return 0;
	}
	return 1;
}

int cr8r_default_cmp_cstr(const cr8r_base_ft *ft, const void *_a, const void *_b){
	return strcmp(*(const char**)_a, *(const char**)_b);
}

int cr8r_default_replace(cr8r_base_ft *ft, void *_a, void *_b){
	memcpy(_a, _b, ft->size);
	return 1;
}

void cr8r_default_free(cr8r_base_ft *ft, void *_p){
	free(*(void**)_p);
}

void cr8r_default_copy_cstr(cr8r_base_ft *ft, void *dest, const void *src){
	*(char**)dest = strdup(*(const char**)src);
}

cr8r_hashtbl_ft cr8r_htft_u64_void = {
	.base = {
		.data = NULL,
		.size = sizeof(uint64_t)
	},
	.hash = cr8r_default_hash_u64,
	.cmp = cr8r_default_cmp_u64,
	.load_factor = .7
};

cr8r_hashtbl_ft cr8r_htft_cstr_u64 = {
	.base.data = NULL,
	.base.size = sizeof(cr8r_htent_cstr_u64),
	.hash = cr8r_default_hash_cstr,
	.cmp = cr8r_default_cmp_cstr,
	.load_factor = .5
};

cr8r_hashtbl_ft cr8r_htft_u64_u64 = {
	.base.data = NULL,
	.base.size = 2*sizeof(uint64_t),
	.hash = cr8r_default_hash_u64,
	.cmp = cr8r_default_cmp_u64,
	.load_factor = .7
};

bool cr8r_hash_ft_init(cr8r_hashtbl_ft *ft,
	void *data, uint64_t size,
	uint64_t (*hash)(const cr8r_base_ft*, const void*),
	int (*cmp)(const cr8r_base_ft*, const void*, const void*),
	int (*add)(cr8r_base_ft*, void*, void*),
	void (*del)(cr8r_base_ft*, void*)
){
	if(!hash || !cmp){
		return false;
	}
	ft->base.data = data;
	ft->base.size = size;
	ft->hash = hash;
	ft->cmp = cmp;
	ft->add = add;
	ft->del = del;
	return true;
}


int cr8r_hash_init(cr8r_hashtbl_t *self, const cr8r_hashtbl_ft *ft, uint64_t reserve){
	*self = (cr8r_hashtbl_t){};
	if(!reserve){
		return 1;
	}
	uint64_t i = 64 - __builtin_clzll(reserve - 1);
	self->len_a = exp_primes[i];
	self->table_a = calloc(ft->base.size, self->len_a);
	if(!self->table_a){
		return 0;
	}
	self->flags_a = calloc(sizeof(uint64_t), ((self->len_a - 1) >> 5) + 1);
	if(!self->flags_a){
		free(self->table_a);
		self->table_a = NULL;
		return 0;
	}
	self->cap = self->len_a*(long double)ft->load_factor;
	return 1;
}

inline static uint64_t cr8r_hash_find_slot_re(cr8r_hashtbl_t *self, const cr8r_hashtbl_ft *ft, uint64_t a){
	for(uint64_t j = 0, i; j < self->len_a; ++j){
		i = (a + (j&1 ? j*j : self->len_a - j*j%self->len_a))%self->len_a;
		uint64_t f = (self->flags_a[i >> 5] >> (i&0x1F))&0x100000001ULL;
		if(!(f&0x100000000ULL)){
			return i;
		}
	}
	return ~0ULL;
}

inline static uint64_t cr8r_hash_find_slot_ap(cr8r_hashtbl_t *self, const cr8r_hashtbl_ft *ft, const void *key){
	uint64_t a = ft->hash(&ft->base,key) % self->len_a;
	for(uint64_t j = 0, i; j < self->len_a; ++j){
		i = (a + (j&1 ? j*j : self->len_a - j*j%self->len_a))%self->len_a;
		uint64_t f = (self->flags_a[i >> 5] >> (i&0x1F))&0x100000001ULL;
		if(!(f&0x100000000ULL && ft->cmp(&ft->base, key, self->table_a + i*ft->base.size))){
			return i;
		}
	}
	return ~0ULL;
}

inline static int cr8r_hash_ix_start(cr8r_hashtbl_t *self, const cr8r_hashtbl_ft *ft){
	uint64_t i = self->len_a ? 64 - __builtin_clzll(self->len_a - 1) : 1;
	uint64_t new_len = exp_primes[i];
	void *new_table = calloc(ft->base.size, new_len);
	if(!new_table){
		return 0;
	}
	uint64_t *new_flags = calloc(sizeof(uint64_t), ((new_len - 1) >> 5) + 1);
	if(!new_flags){
		free(new_table);
		return 0;
	}
	uint64_t new_cap = new_len*(double)ft->load_factor;
	//we currently have full entries and can fit new_cap before resizing.  This means we must move full/(new_cap - full)
	//entries on average per insertion.  Because full == cap + 1, new_cap == new_len*load_factor, cap == len_a*load_factor,
	//and new_len == len_a*growth_factor, full/(new_cap - full) ~~ cap/(new_cap - cap) ~~ len_a/(new_len - len_a)
	//~~ len_a/(len_a*growth_factor - len_a) ~~ 1/(growth_factor - 1).  I picked the largest prime number before each power
	//of 2 so full/(new_cap - full) is very close to 1.  It is always less than 2 so I might just use 2 instead of r
	#ifdef DEBUG
	if(self->table_b){
		abort();
	}
	#endif
	self->table_b = self->table_a;
	self->table_a = new_table;
	self->flags_b = self->flags_a;
	self->flags_a = new_flags;
	self->len_b = self->len_a;
	self->len_a = new_len;
	self->cap = new_cap;
	self->r = ceill((self->full + 1.)/(new_cap - self->full - 1.));
	self->i = 0;
	return 1;
}

inline static void cr8r_hash_ix_move(cr8r_hashtbl_t *self, const cr8r_hashtbl_ft *ft, uint64_t n){
	uint64_t b = self->i;
	for(uint64_t a; b < self->len_b; ++b){
		uint64_t f = (self->flags_b[b >> 5] >> (b&0x1F))&0x100000001ULL;
		if(!(f&0x100000000ULL)){
			continue;
		}
		void *ent = self->table_b + b*ft->base.size;
		a = ft->hash(&ft->base,ent) % self->len_a;
		a = cr8r_hash_find_slot_re(self, ft, a);
		memcpy(self->table_a + a*ft->base.size, ent, ft->base.size);
		self->flags_b[b >> 5] |= 0x1ULL << (b&0x1F);
		self->flags_b[b >> 5] &= ~(0x100000000ULL << (b&0x1F));
		self->flags_a[a >> 5] |= 0x100000000ULL << (a&0x1F);
		self->flags_a[a >> 5] &= ~(0x1ULL << (a&0x1F));
		if(!--n){
			break;
		}
	}
	self->i = b;
	if(b == self->len_b){
		free(self->table_b);
		free(self->flags_b);
		self->table_b = NULL;
		self->flags_b = NULL;
		self->len_b = 0;
		self->i = 0;
	}
}

inline static uint64_t cr8r_hash_get_index(void *table, uint64_t *flags, uint64_t len, uint64_t a, const cr8r_hashtbl_ft *ft, const void *key){
	a %= len;
	for(uint64_t j = 0, i; j < len; ++j){
		i = (a + (j&1 ? j*j : len - j*j%len))%len;
		uint64_t f = (flags[i >> 5] >> (i&0x1F))&0x100000001ULL;
		if(!f){
			return ~0ULL;
		}
		if(f&0x100000000ULL && !ft->cmp(&ft->base, key, table + i*ft->base.size)){
			return i;
		}
	}
	return ~0ULL;
}

inline static void *cr8r_hash_get_single_a(cr8r_hashtbl_t *self, uint64_t i, const cr8r_hashtbl_ft *ft, const void *key){
	uint64_t a = cr8r_hash_get_index(self->table_a, self->flags_a, self->len_a, i, ft, key);
	return ~a ? self->table_a + a*ft->base.size : NULL;
}

inline static void *cr8r_hash_get_single_b(cr8r_hashtbl_t *self, uint64_t i, const cr8r_hashtbl_ft *ft, const void *key){
	uint64_t b = cr8r_hash_get_index(self->table_b, self->flags_b, self->len_b, i, ft, key);
	return ~b ? self->table_b + b*ft->base.size : NULL;
}

inline static void *cr8r_hash_get_split(cr8r_hashtbl_t *self, const cr8r_hashtbl_ft *ft, const void *key){
	uint64_t i = ft->hash(&ft->base,key);
	if(self->i << 1 < self->len_b){
		return cr8r_hash_get_single_a(self, i, ft, key) ?: cr8r_hash_get_single_b(self, i, ft, key);
	}
	return cr8r_hash_get_single_b(self, i, ft, key) ?: cr8r_hash_get_single_a(self, i, ft, key);
}

void *cr8r_hash_get(cr8r_hashtbl_t *self, const cr8r_hashtbl_ft *ft, const void *key){
	if(!self->table_a){
		return NULL;
	}
	return self->table_b ? cr8r_hash_get_split(self, ft, key) : cr8r_hash_get_single_a(self, ft->hash(&ft->base,key), ft, key);
}

void *cr8r_hash_insert(cr8r_hashtbl_t *self, const cr8r_hashtbl_ft *ft, const void *key, int *_status){
	void *ret = NULL;
	int status = 0;
	if(self->full == self->cap){
		if(!cr8r_hash_ix_start(self, ft)){
			if(_status){
				*_status = 0;
			}
			return NULL;
		}
	}
	if(self->table_b){
		cr8r_hash_ix_move(self, ft, self->r);
		if(self->table_b){
			if(cr8r_hash_get_single_b(self, ft->hash(&ft->base,key), ft, key)){
				if(_status){
					*_status = 2;
				}
				return NULL;
			}
		}
	}
	uint64_t i = cr8r_hash_find_slot_ap(self, ft, key);
	if(!~i){
		if(_status){
			*_status = 0;
		}
		return NULL;
	}
	if((self->flags_a[i >> 5] >> (i&0x1F))&0x100000000ULL){
		status = 2;
	}else{
		ret = self->table_a + i*ft->base.size;
		memcpy(ret, key, ft->base.size);
		self->flags_a[i >> 5] |= 0x100000000ULL << (i&0x1F);
		self->flags_a[i >> 5] &= ~(0x1ULL << (i&0x1F));
		++self->full;
		status = 1;
	}
	if(_status){
		*_status = status;
	}
	return ret;
}

void *cr8r_hash_append(cr8r_hashtbl_t *self, cr8r_hashtbl_ft *ft, void *key, int *_status){
	void *ret = NULL;
	int status = 0;
	if(self->full == self->cap){
		if(!cr8r_hash_ix_start(self, ft)){
			if(_status){
				*_status = 0;
			}
			return NULL;
		}
	}
	if(self->table_b){
		cr8r_hash_ix_move(self, ft, self->r);
		if(self->table_b){
			if((ret = cr8r_hash_get_single_b(self, ft->hash(&ft->base,key), ft, key))){
				status = ft->add(&ft->base,ret, key) ? 2 : 0;
				if(_status){
					*_status = status;
				}
				return ret;
			}
		}
	}
	uint64_t i = cr8r_hash_find_slot_ap(self, ft, key);
	if(!~i){
		if(_status){
			*_status = 0;
		}
		return NULL;
	}
	ret = self->table_a + i*ft->base.size;
	if((self->flags_a[i >> 5] >> (i&0x1F))&0x100000000ULL){
		status = ft->add(&ft->base,ret, key) ? 2 : 0;
	}else{
		memcpy(ret, key, ft->base.size);
		self->flags_a[i >> 5] |= 0x100000000ULL << (i&0x1F);
		self->flags_a[i >> 5] &= ~(0x1ULL << (i&0x1F));
		++self->full;
		status = 1;
	}
	if(_status){
		*_status = status;
	}
	return ret;
}

inline static int cr8r_hash_remove_single_a(cr8r_hashtbl_t *self, uint64_t i, cr8r_hashtbl_ft *ft, const void *key){
	uint64_t a = cr8r_hash_get_index(self->table_a, self->flags_a, self->len_a, i, ft, key);
	if(!~a){
		return 0;
	}
	self->flags_a[a >> 5] ^= 0x100000001ULL << (a&0x1F);
	--self->full;
	if(ft->del){
		ft->del(&ft->base, self->table_a + a*ft->base.size);
	}
	return 1;
}

inline static int cr8r_hash_remove_single_b(cr8r_hashtbl_t *self, uint64_t i, cr8r_hashtbl_ft *ft, const void *key){
	uint64_t b = cr8r_hash_get_index(self->table_b, self->flags_b, self->len_b, i, ft, key);
	if(!~b){
		return 0;
	}
	self->flags_b[b >> 5] ^= 0x100000001ULL << (b&0x1F);
	--self->full;
	if(ft->del){
		ft->del(&ft->base, self->table_b + b*ft->base.size);
	}
	return 1;
}

inline static int cr8r_hash_remove_split(cr8r_hashtbl_t *self, cr8r_hashtbl_ft *ft, const void *key){
	uint64_t i = ft->hash(&ft->base,key);
	if(self->i << 1 < self->len_b){
		return cr8r_hash_remove_single_a(self, i, ft, key) ?: cr8r_hash_remove_single_b(self, i, ft, key);
	}
	return cr8r_hash_remove_single_b(self, i, ft, key) ?: cr8r_hash_remove_single_a(self, i, ft, key);
}

int cr8r_hash_remove(cr8r_hashtbl_t *self, cr8r_hashtbl_ft *ft, const void *key){
	if(!self->table_a){
		return 0;
	}
	return self->table_b ? cr8r_hash_remove_split(self, ft, key) : cr8r_hash_remove_single_a(self, ft->hash(&ft->base,key), ft, key);
}

void cr8r_hash_delete(cr8r_hashtbl_t *self, cr8r_hashtbl_ft *ft, void *ent){
	if(self->table_a <= ent && ent < self->table_a + self->len_a*ft->base.size){
		uint64_t i = (ent - self->table_a)/ft->base.size;
		self->flags_a[i >> 5] ^= 0x100000001ULL << (i&0x1F);
	}else{
		uint64_t i = (ent - self->table_b)/ft->base.size;
		self->flags_b[i >> 5] ^= 0x100000001ULL << (i&0x1F);
	}
	--self->full;
	if(ft->del){
		ft->del(&ft->base,ent);
	}
}

void cr8r_hash_clear(cr8r_hashtbl_t *self, cr8r_hashtbl_ft *ft){
	if(ft->del){
		for(uint64_t a = 0; a < self->len_a; ++a){
			uint64_t f = (self->flags_a[a >> 5] >> (a&0x1F))&0x100000001ULL;
			if(f&0x100000000ULL){
				ft->del(&ft->base, self->table_a + a*ft->base.size);
			}
		}
		for(uint64_t b = 0; b < self->len_b; ++b){
			uint64_t f = (self->flags_b[b >> 5] >> (b&0x1F))&0x100000001ULL;
			if(f&0x100000000ULL){
				ft->del(&ft->base, self->table_b + b*ft->base.size);
			}
		}
	}
	free(self->table_b);
	self->table_b = NULL;
	free(self->flags_b);
	self->flags_b = NULL;
	self->len_b = 0;
	memset(self->flags_a, 0, (((self->len_a - 1) >> 5) + 1)*sizeof(uint64_t));
	self->full = 0;
}

void cr8r_hash_destroy(cr8r_hashtbl_t *self, cr8r_hashtbl_ft *ft){
	cr8r_hash_clear(self, ft);
	free(self->table_a);
	self->table_a = NULL;
	free(self->flags_a);
	self->flags_a = NULL;
	self->len_a = 0;
	self->cap = 0;
}

