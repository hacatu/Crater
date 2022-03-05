#include "crater/prand.h"
#include <stdint.h>
#include <string.h>

#include <crater/bitvec.h>

bool cr8r_bvec_init(cr8r_bvec *self, cr8r_bvec_ft *ft, uint64_t bits){
	uint64_t cap = (bits + 63)/64;
	uint64_t *tmp = ft->resize(&ft->base, NULL, cap);
	if(!tmp){
		return !cap;
	}
	*self = (cr8r_bvec){.buf=tmp, .cap = 64*cap};
	return 1;
}

void cr8r_bvec_delete(cr8r_bvec *self, cr8r_bvec_ft *ft){
	ft->resize(&ft->base, self->buf, 0);
	*self = (cr8r_bvec){};
}

bool cr8r_bvec_copy(cr8r_bvec *dest, const cr8r_bvec *src, cr8r_bvec_ft *ft){
	if(!cr8r_bvec_init(dest, ft, src->len)){
		return 0;
	}
	memcpy(dest->buf, src->buf, dest->cap/64*sizeof(uint64_t));
	return 1;
}

bool cr8r_bvec_sub(cr8r_bvec *dest, const cr8r_bvec *src, cr8r_bvec_ft *ft, uint64_t a, uint64_t b){
	if(a > b || b > src->len || !cr8r_bvec_init(dest, ft, b - a)){
		return 0;
	}else if(a == b){
		*dest = (cr8r_bvec){};
		return 1;
	}
	uint64_t cap = (b - a + 63)/64;
	uint64_t offset = a%64;
	uint64_t src_base = a/64;
	if(!offset){
		memcpy(dest->buf, src->buf + src_base, cap*sizeof(uint64_t));
		return 1;
	}
	for(uint64_t i = 0; i < cap - 1; ++i){
		dest->buf[i] = (src->buf[src_base + i] >> offset) | (src->buf[src_base + i + 1] << (64 - offset));
	}
	dest->buf[cap - 1] = src->buf[src_base + cap - 1] >> offset;
	if(b/64 >= cap){
		dest->buf[cap - 1] |= src->buf[src_base + cap] << (64 - offset);
	}
	return 1;
}

bool cr8r_bvec_resize(cr8r_bvec *self, cr8r_bvec_ft *ft, uint64_t bits){
	if(bits < self->len){
		return 0;
	}
	uint64_t cap = (bits + 63)/64;
	if(self->cap == 64*cap){
		return 1;
	}
	uint64_t *tmp = ft->resize(&ft->base, self->buf, cap);
	if(!tmp){
		return !bits;
	}
	self->cap = 64*cap;
	self->buf = tmp;
	return 1;
}

bool cr8r_bvec_trim(cr8r_bvec *self, cr8r_bvec_ft *ft){
	uint64_t cap = (self->len + 63)/64;
	if(self->cap == 64*cap){
		return 1;
	}
	uint64_t *tmp = ft->resize(&ft->base, self->buf, cap);
	if(!tmp){
		if(!self->len){
			self->cap = 0;
			self->buf = NULL;
			return 1;
		}
		return 0;
	}
	self->cap = 64*cap;
	self->buf = tmp;
	return 1;
}

void cr8r_bvec_clear(cr8r_bvec *self){
	self->len = 0;
}

// Both this and forall permutations have to deal with, well, permutations of
// an n bit long bit vector.  In a vector containing n regular objects,
// there are n! (n factorial) permutations.  However, this assumes that
// all the objects are unique.  This is why regular objects (in many languages
// such as C++, Java, Python, Rust, Javascript, and just about any language with
// objects) are said to have "identity semantics", and sometimes certain
// classes can be said to have "value semantics", meaning two objects with
// the same representation or comparing equal are interchangeable.  All objects
// within vectors, and indeed most containers, have at least some degree of
// identity semantics, because even if the container is storing primitives,
// each one takes up its own space in its own location.
//
// When working with a vector of objects with value semantics which have few
// possible values so that many repeats are likely, special algorithms that
// take advantage of the perfect fungability of identical objects can outperform
// general purpose algorithms.
//
// We know the number of permutations of n identity semantic objects is n!.
// Bits represent the other extreme, not only do they have value semantics but
// they only have two possible values.  So the number of permutations of n bits
// with k set is (n choose k).
//
// There is a method to convert a bit string of length n with k bits set
// to and from an ordinal on [0, nCk).  This method is called "enumerative coding",
// but I don't think it would really be beneficial for large bit vectors, which are
// the only kind it makes sense to optimize for.  The main reason for this is
// that it requires computing a lot of binomial coefficients for large n and k.
// These are only O(1) amortized multiplies/divides, but these have to use bignums,
// making enumerative coding less appealing for large bit vectors.
//
// The existing algorithm simply counts the number of bits set (required by any
// shuffling algorithm that isn't given it), unsets all bits, then chooses
// non-set bits randomly and sets them if not set until k have been set.  If there
// are more zeros than ones, we set all bits and pick n-k to zero out instead.
// This ensures that the expected number of random numbers needed is below n.
void cr8r_bvec_shuffle(cr8r_bvec *self, cr8r_prng *prng){
	uint64_t num_ones = cr8r_bvec_popcount(self);
	if(2*num_ones > self->len){
		if(num_ones == self->len){
			return;
		}
		cr8r_bvec_set_range(self, 0, self->len, 1);
		for(uint64_t num_zeros = self->len - num_ones, i; num_zeros-- > 0;){
			do{
				i = cr8r_prng_uniform_u64(prng, 0, self->len);
			}while(!cr8r_bvec_getu(self, i));
			cr8r_bvec_setu(self, i, 0);
		}
	}else{
		if(!num_ones){
			return;
		}
		cr8r_bvec_set_range(self, 0, self->len, 0);
		for(uint64_t i; num_ones-- > 0;){
			do{
				i = cr8r_prng_uniform_u64(prng, 0, self->len);
			}while(cr8r_bvec_getu(self, i));
			cr8r_bvec_setu(self, i, 1);
		}
	}
}

bool cr8r_bvec_get(cr8r_bvec *self, uint64_t i){
	if(i >= self->len){
		return 0;
	}
	return self->buf[i/64] & (1ull << i%64);
}

bool cr8r_bvec_getu(cr8r_bvec *self, uint64_t i){
	return self->buf[i/64] & (1ull << i%64);
}

bool cr8r_bvec_getx(cr8r_bvec *self, int64_t i){
	if(i < 0){
		i += self->len;
	}
	if((uint64_t)i >= self->len){
		return 0;
	}
	return self->buf[i/64] & (1ull << i%64);
}

bool cr8r_bvec_getux(cr8r_bvec *self, int64_t i){
	if(i < 0){
		i += self->len;
	}
	return self->buf[i/64] & (1ull << i%64);
}

bool cr8r_bvec_set(cr8r_bvec *self, uint64_t i, bool b){
	if(i >= self->len){
		return 0;
	}else if(b){
		self->buf[i/64] |= 1ull << i%64;
	}else{
		self->buf[i/64] &= ~(1ull << i%64);
	}
	return 1;
}

void cr8r_bvec_setu(cr8r_bvec *self, uint64_t i, bool b){
	if(b){
		self->buf[i/64] |= 1ull << i%64;
	}else{
		self->buf[i/64] &= ~(1ull << i%64);
	}
}

bool cr8r_bvec_setx(cr8r_bvec *self, int64_t i, bool b){
	if(i < 0){
		i += self->len;
	}
	if((uint64_t)i >= self->len){
		return 0;
	}else if(b){
		self->buf[i/64] |= 1ull << i%64;
	}else{
		self->buf[i/64] &= ~(1ull << i%64);
	}
	return 1;
}

void cr8r_bvec_setux(cr8r_bvec *self, int64_t i, bool b){
	if(i < 0){
		i += self->len;
	}
	if(b){
		self->buf[i/64] |= 1ull << i%64;
	}else{
		self->buf[i/64] &= ~(1ull << i%64);
	}
}

uint64_t cr8r_bvec_len(cr8r_bvec *self){
	return self->len;
}

bool cr8r_bvec_pushr(cr8r_bvec *self, cr8r_bvec_ft *ft, bool b){
	if(self->len + 1 > self->cap){
		uint64_t cap = ft->new_size(&ft->base, self->cap/64);
		if(64*cap < self->len + 1){
			return 0;
		}
		uint64_t *tmp = ft->resize(&ft->base, self->buf, cap);
		if(!tmp){
			return 0;
		}
		self->cap = 64*cap;
		self->buf = tmp;
	}
	cr8r_bvec_setu(self, self->len++, b);
	return 1;
}

bool cr8r_bvec_popr(cr8r_bvec *self, int *status){
	if(!self->len){
		if(status){
			*status = 0;
		}
		return 0;
	}
	bool res = cr8r_bvec_getu(self, --self->len);
	if(status){
		*status = 1;
	}
	return res;
}

bool cr8r_bvec_pushl(cr8r_bvec *self, cr8r_bvec_ft *ft, bool b){
	if(self->len + 1 > self->cap){
		uint64_t cap = ft->new_size(&ft->base, self->cap/64);
		if(64*cap < self->len + 1){
			return 0;
		}
		uint64_t *tmp = ft->resize(&ft->base, self->buf, cap);
		if(!tmp){
			return 0;
		}
		self->cap = 64*cap;
		self->buf = tmp;
	}
	uint64_t cap = (self->len + 63)/64;
	if(self->len%64 == 0){
		self->buf[cap] = self->buf[cap - 1] >> 63;
	}
	while(--cap > 0){
		self->buf[cap] = (self->buf[cap] << 1) | (self->buf[cap - 1] >> 63);
	}
	self->buf[0] = (self->buf[0] << 1) | (uint64_t)b;
	++self->len;
	return 1;
}

bool cr8r_bvec_popl(cr8r_bvec *self, int *status){
	if(!self->len){
		if(status){
			*status = 0;
		}
		return 0;
	}
	bool res = self->buf[0] & 1;
	for(uint64_t i = 0; i + 1 < self->cap/64; ++i){
		self->buf[i] = (self->buf[i] >> 1) | (self->buf[i + 1] << 63);
	}
	self->buf[self->cap/64 - 1] >>= 1;
	--self->len;
	if(status){
		*status = 1;
	}
	return res;
}

// This algorithm is adapted from the classic bit twiddling hacks page
// (https://graphics.stanford.edu/%7Eseander/bithacks.html#NextBitPermutation)
// The algorithm presented there converts a permutation "v" into the next permutation "w"
// t = v | (v - 1) // t gets v's least significant 0 bits set to 1
// // Next set to 1 the most significant bit to change, 
// // set to 0 the least significant ones, and add the necessary 1 bits.
// w = (t + 1) | (((~t & -~t) - 1) >> (ctz(v) + 1))
// To unpack this, we need to recognize a few things.  The comments and existing knowledge of
// bit twiddling and this problem in particular are helpful.
// First of all, t is v with any trailing zeros set to 1.
// Then t + 1 is v with the first zero in the second chunk of zeros (from least significant)
// set to 1, and all less significant bits set to 0.
// Next, recall that -x = ~(x-1), which in particular lets us rewrite
// -~t as --(t+1) = (t+1).  I'm not sure why that was not done in the code on the site,
// w = (t+1) | (((~t & (t+1)) - 1) >> (ctz(v) + 1))
// Now we can recognize that t and (t+1) have the same bits after (more significant than)
// the newly set 1, so ~t & (t+1) is just zero above that bit.  Similarly, (t+1) is zero
// below that bit, so (~t & (t+1)) represents just the newly set 1 bit.  Thus the entire
// expression on the rhs of the | operator represents moving all the ones in the first
// (least significant) chunk, besides the one moved up to become the newly set bit,
// to the least significant position.
//
// We can break this down into operations more amenable to those supported by bit vectors
// as follows:
// tz = ctz(v)
// v.set_range(0, tz, 1) // this sets v to effectively t in the above algorithm
// to = cto(v)
// v.set(to, 1) // this sets the "newly set" 1 bit
// v.set_range(to - tz - 1, to, 0) // this effectively moves the rest of the bits in the first
// // chunk of ones to the beginning.
//
// In the first algorithm, the final permutation can be detected when t+1 overflows.
// Similarly in the second algorithm, the final permutation can be detected when cto(v) == v.len.
//
// Compare the choices made in this function to the choices made in cr8r_bvec_shuffle.
// Finally, notice that considering choosing n-k zeros out of n positions instead of
// k ones out of n positions probably would not help here, so I didn't do it.
int cr8r_bvec_forEachPermutation(cr8r_bvec *self, void (*f)(const cr8r_bvec*, void *data), void *data){
	// first we set the bit vector to the lexicographically first one with the same number of bits set.
	uint64_t popcount = cr8r_bvec_popcount(self);
	// if the bit vector has all its bits set however, it only has one permutation so we don't have to
	// do anything but call the callback and finish.
	if(popcount == self->len){
		f(self, data);
		return 1;
	}
	cr8r_bvec_set_range(self, 0, popcount, 1);
	cr8r_bvec_set_range(self, popcount, self->len, 0);
	while(true){
		uint64_t tz = cr8r_bvec_ctz(self);
		cr8r_bvec_set_range(self, 0, tz, 1);
		uint64_t to = cr8r_bvec_cto(self);
		if(to == self->len){
			cr8r_bvec_set_range(self, 0, tz, 0);
			return 1;
		}
		cr8r_bvec_setu(self, to, 1);
		cr8r_bvec_set_range(self, to - tz - 1, to, 0);
		f(self, data);
	}
}

bool cr8r_bvec_combine(cr8r_bvec *dest, const cr8r_bvec *src_a, const cr8r_bvec *src_b, cr8r_bvec_ft *ft){
	if(!cr8r_bvec_init(dest, ft, src_a->len + src_b->len)){
		return 0;
	}
	memcpy(dest->buf, src_a->buf, src_a->cap/64*sizeof(uint64_t));
	dest->len = src_a->len;
	return cr8r_bvec_augment(dest, src_b, ft);
}

bool cr8r_bvec_augment(cr8r_bvec *self, const cr8r_bvec *other, cr8r_bvec_ft *ft){
	if(self->len + other->len > self->cap){
		uint64_t cap = ft->new_size(&ft->base, self->cap/64);
		if(self->len + other->len > 64*cap){
			cap = (self->len + other->len + 63)/64*64;
		}
		if(!cr8r_bvec_resize(self, ft, cap)){
			return 0;
		}
	}
	if(self->len%64 == 0){
		memcpy(self->buf + self->len/64*sizeof(uint64_t), other->buf, (other->len + 63)/64*sizeof(uint64_t));
		self->len += other->len;
		return 1;
	}
	uint64_t offset = self->len%64;
	uint64_t self_base = self->len/64;
	self->buf[self_base] &= (uint64_t)-1 >> (64 - offset);
	self->buf[self_base] |= other->buf[0] << offset;
	uint64_t cap = (other->len + 63)/64;
	for(uint64_t i = 0; i + 1 < cap; ++i){
		self->buf[self_base + i + 1] = (other->buf[i] >> (64 - offset)) | (other->buf[i + 1] << offset);
	}
	self->buf[self_base + cap - 1] = other->buf[cap - 1] >> (64 - offset);
	self->len += other->len;
	return 1;
}

bool cr8r_bvec_all(const cr8r_bvec *self){
	uint64_t l = self->len/64;
	uint64_t w = self->len%64;
	for(uint64_t i = 0; i < l; ++i){
		if(~self->buf[i]){
			return 0;
		}
	}
	if(w){
		uint64_t mask = (uint64_t)-1 >> (64 - w);
		return (self->buf[l] & mask) == mask;
	}
	return 1;
}

bool cr8r_bvec_any(const cr8r_bvec *self){
	uint64_t l = self->len/64;
	uint64_t w = self->len%64;
	for(uint64_t i = 0; i < l; ++i){
		if(self->buf[i]){
			return 1;
		}
	}
	if(w){
		uint64_t mask = (uint64_t)-1 >> (64 - w);
		return self->buf[l] & mask;
	}
	return 0;
}

uint64_t cr8r_bvec_popcount(const cr8r_bvec *self){
	uint64_t l = self->len/64;
	uint64_t w = self->len%64;
	uint64_t res = 0;
	for(uint64_t i = 0; i < l; ++i){
		res += __builtin_popcountll(self->buf[i]);
	}
	if(w){
		uint64_t mask = (uint64_t)-1 >> (64 - w);
		res += __builtin_popcountll(self->buf[l] & mask);
	}
	return res;
}

uint64_t cr8r_bvec_clz(const cr8r_bvec *self){
	uint64_t l = self->len/64;
	uint64_t w = self->len%64;
	uint64_t res = 0;
	if(w){
		uint64_t mask = (uint64_t)-1 >> w;
		res = __builtin_clzll((self->buf[l] << w) | mask);
	}
	for(uint64_t i = l; i-- > 0;){
		if(!self->buf[i]){
			res += 64;
		}else{
			return res + __builtin_clzll(self->buf[i]);
		}
	}
	return res;
}

uint64_t cr8r_bvec_ctz(const cr8r_bvec *self){
	uint64_t l = self->len/64;
	uint64_t w = self->len%64;
	uint64_t res = 0;
	for(uint64_t i = 0; i < l; ++i){
		if(!self->buf[i]){
			res += 64;
		}else{
			return res + __builtin_ctzll(self->buf[i]);
		}
	}
	if(w){
		uint64_t mask = (uint64_t)-1 << w;
		res += __builtin_ctzll(self->buf[l] | mask);
	}
	return res;
}

uint64_t cr8r_bvec_clo(const cr8r_bvec *self){
	uint64_t l = self->len/64;
	uint64_t w = self->len%64;
	uint64_t res = 0;
	if(w){
		res = __builtin_clzll(~(self->buf[l] << w));
	}
	for(uint64_t i = l; i-- > 0;){
		if(!~self->buf[i]){
			res += 64;
		}else{
			return res + __builtin_clzll(~self->buf[i]);
		}
	}
	return res;
}

uint64_t cr8r_bvec_cto(const cr8r_bvec *self){
	uint64_t l = self->len/64;
	uint64_t w = self->len%64;
	uint64_t res = 0;
	for(uint64_t i = 0; i < l; ++i){
		if(!~self->buf[i]){
			res += 64;
		}else{
			return res + __builtin_ctzll(~self->buf[i]);
		}
	}
	if(w){
		uint64_t mask = (uint64_t)-1 << w;
		res += __builtin_ctzll(~self->buf[l] | mask);
	}
	return res;
}

void cr8r_bvec_icompl(cr8r_bvec *self){
	uint64_t cap = (self->len + 63)/64;
	for(uint64_t i = 0; i < cap; ++i){
		self->buf[i] ^= (uint64_t)-1;
	}
}

void cr8r_bvec_iand(cr8r_bvec *self, const cr8r_bvec *other){
	uint64_t cap = other->len < self->len ? other->len : self->len;
	uint64_t l = cap/64;
	uint64_t w = cap%64;
	for(uint64_t i = 0; i < l; ++i){
		self->buf[i] &= other->buf[i];
	}
	if(w){
		self->buf[l] &= other->buf[l];
	}
	if(other->len < self->len){
		cr8r_bvec_set_range(self, other->len, self->len, 0);
	}
}

void cr8r_bvec_ior(cr8r_bvec *self, const cr8r_bvec *other){
	uint64_t cap = other->len < self->len ? other->len : self->len;
	uint64_t l = cap/64;
	uint64_t w = cap%64;
	for(uint64_t i = 0; i < l; ++i){
		self->buf[i] |= other->buf[i];
	}
	if(w){
		self->buf[l] |= other->buf[l] & ((uint64_t)-1 >> (64-w));
	}
}

void cr8r_bvec_ixor(cr8r_bvec *self, const cr8r_bvec *other){
	uint64_t cap = other->len < self->len ? other->len : self->len;
	uint64_t l = cap/64;
	uint64_t w = cap%64;
	for(uint64_t i = 0; i < l; ++i){
		self->buf[i] ^= other->buf[i];
	}
	if(w){
		self->buf[l] ^= other->buf[l] & ((uint64_t)-1 >> (64-w));
	}
}

bool cr8r_bvec_any_range(const cr8r_bvec *self, uint64_t a, uint64_t b){
	if(a >= b || b > self->len){
		return 0;
	}
	uint64_t al = a/64;
	uint64_t aw = a%64;
	uint64_t bl = b/64;
	uint64_t bw = b%64;
	if(aw){
		if(al == bl && bw){
			return self->buf[al] & ((uint64_t)-1 >> (64 - bw)) & ((uint64_t)-1 << aw);
		}else if(self->buf[al] & ((uint64_t)-1 << aw)){
			return 1;
		}
		++al;
	}
	for(; al < bl; ++al){
		if(self->buf[al]){
			return 1;
		}
	}
	return bw && (self->buf[bl] & ((uint64_t)-1 >> (64 - bw)));
}

bool cr8r_bvec_all_range(const cr8r_bvec *self, uint64_t a, uint64_t b){
	if(a > b || b > self->len){
		return 0;
	}else if(a == b){
		return 1;
	}
	uint64_t al = a/64;
	uint64_t aw = a%64;
	uint64_t bl = b/64;
	uint64_t bw = b%64;
	uint64_t mask = (uint64_t)-1 << aw;
	if(aw){
		if(al == bl && bw){
			mask &= (uint64_t)-1 >> (64 - bw);
			return (self->buf[al] & mask) == mask;
		}else if((self->buf[al] & mask) != mask){
			return 0;
		}
		++al;
	}
	for(; al < bl; ++al){
		if(~self->buf[al]){
			return 0;
		}
	}
	mask = (uint64_t)-1 >> (64 - bw);
	return !bw || (self->buf[bl] & mask) == mask;
}

bool cr8r_bvec_set_range(cr8r_bvec *self, uint64_t a, uint64_t b, bool v){
	if(b < a || b > self->len){
		return 0;
	}
	uint64_t al = a/64;
	uint64_t aw = a%64;
	uint64_t bl = b/64;
	uint64_t bw = b%64;
	if(v){
		if(aw){
			if(al == bl && bw){
				self->buf[al] |= ((uint64_t)-1 >> (64 - bw)) & ((uint64_t)-1 << aw);
				return 1;
			}
			self->buf[al] |= (uint64_t)-1 << aw;
			++al;
		}
		if(al != bl){
			memset(self->buf + al, 0xFF, (bl - al)*sizeof(uint64_t));
		}
		if(bw){
			self->buf[bl] |= (uint64_t)-1 >> (64 - bw);
		}
	}else{
		if(aw){
			if(al == bl && bw){
				self->buf[al] &= ~(((uint64_t)-1 >> (64 - bw)) & ((uint64_t)-1 << aw));
				return 1;
			}
			self->buf[al] &= (uint64_t)-1 >> (64 - aw);
			++al;
		}
		if(al != bl){
			memset(self->buf + al, 0, (bl - al)*sizeof(uint64_t));
		}
		if(bw){
			self->buf[bl] &= (uint64_t)-1 << bw;
		}
	}
	return 1;
}

int cr8r_bvec_cmp(const cr8r_bvec *a, const cr8r_bvec *b){
	uint64_t a_width = a->len - cr8r_bvec_clz(a);
	uint64_t b_width = b->len - cr8r_bvec_clz(b);
	if(a_width > b_width){
		return 1;
	}else if(a_width < b_width){
		return -1;
	}
	uint64_t l = a_width/64;
	uint64_t w = a_width%64;
	if(w){
		uint64_t mask = (uint64_t)-1 >> (64 - w);
		uint64_t aa = mask&a->buf[l];
		uint64_t bb = mask&b->buf[l];
		if(aa > bb){
			return 1;
		}else if(aa < bb){
			return -1;
		}
	}
	while(l-- > 0){
		if(a->buf[l] > b->buf[l]){
			return 1;
		}else if(a->buf[l] < b->buf[l]){
			return -1;
		}
	}
	return 0;
}

cr8r_bvec_ft cr8r_bvecft = {.base.size=sizeof(uint64_t)};

