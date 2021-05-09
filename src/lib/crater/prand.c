#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>

#include <crater/prand.h>

bool cr8r_prng_seed(cr8r_prng *self, const char seed[self->state_size]){
	return self->seed(self->state, seed);
}

uint32_t cr8r_prng_get_u32(cr8r_prng *self){
	return self->get_u32(self->state);
}

uint64_t cr8r_prng_get_u64(cr8r_prng *self){
	uint64_t res = self->get_u32(self->state);
	return res | ((uint64_t)self->get_u32(self->state) << 32);
}

void cr8r_prng_get_bytes(cr8r_prng *self, uint64_t size, char buf[static size]){
	uint64_t i;
	for(i = 0; i + sizeof(uint32_t) <= size; i += sizeof(uint32_t)){
		uint32_t tmp = self->get_u32(self->state);
		memcpy(buf + i*sizeof(uint32_t), &tmp, sizeof(uint32_t));
	}
	if(i != size){
		uint32_t tmp = self->get_u32(self->state);
		memcpy(buf + i*sizeof(uint32_t), &tmp, size - i);
	}
}

uint64_t cr8r_prng_uniform_u64(cr8r_prng *self, uint64_t a, uint64_t b){
	uint64_t l = b - a;
	if(!l){
		return a;
	}
	uint64_t one_call = l <= 0x100000000ull;
	uint64_t ub, r;
	if(one_call){
		ub = 0x100000000ull;
		ub -= ub%l;
		do{
			r = cr8r_prng_get_u32(self);
		}while(r >= ub);
	}else{
		ub = 0x7FFFFFFFFFFFFFFFull%l*-2;
		do{
			r = cr8r_prng_get_u64(self);
		}while(r >= ub);
	}
	return r%l + a;
}

static uint32_t cr8r_prng_system_get_u32(void *_state){
	uint32_t res;
	getrandom(&res, sizeof(uint32_t), 0);
	return res;
}

static bool cr8r_prng_system_seed(void *_state, const void *seed){
	return false;
}

cr8r_prng *cr8r_prng_init_system(){
	cr8r_prng *res = malloc(offsetof(cr8r_prng, state));
	if(res){
		res->state_size = 0;
		res->get_u32 = cr8r_prng_system_get_u32;
		res->seed = cr8r_prng_system_seed;
	}
	return res;
}

static uint32_t cr8r_prng_lcg_get_u32(void *_state){
	uint64_t *state = _state;
	*state = 6364136223846793005**state + 1;
	return *state >> 32;
}

static bool cr8r_prng_lcg_seed(void *_state, const void *_seed){
	uint64_t seed = *(const uint64_t*)_seed;
	if(!seed){
		return false;
	}
	*(uint64_t*)_state = seed;
	return true;
}

cr8r_prng *cr8r_prng_init_lcg(uint64_t seed){
	cr8r_prng *res = malloc(offsetof(cr8r_prng, state) + sizeof(uint64_t));
	if(res){
		if(!cr8r_prng_lcg_seed(res->state, &seed)){
			free(res);
			return NULL;
		}
		res->state_size = sizeof(uint64_t);
		res->get_u32 = cr8r_prng_lcg_get_u32;
		res->seed = cr8r_prng_lcg_seed;
	}
	return res;
}

static uint32_t cr8r_prng_lfg_get_u32(void *_state){
	uint64_t x0 = 0, x5 = 0, x12 = 0;
	// depends on little endianness
	memcpy(&x5, _state + 6*4, 6);
	memcpy(&x12, _state + 6*11, 6);
	x0 = x5 - x12 - ((uint8_t*)_state)[6*12];
	// set carry bit to 1 if x0 < 0
	((uint8_t*)_state)[6*12] = x0 >= 0x1000000000000ull;
	memmove(_state + 6, _state, 6*11);
	memcpy(_state, &x0, 6);
	return x0 >> 16;
}

static bool cr8r_prng_lfg_seed(void *_state, const void *_seed){
	bool has_odd = false;
	const char *seed = _seed;
	for(uint64_t i = 0; i < 12; i += 6){
		// depends on little endianness
		if(seed[i]&1){
			has_odd = true;
			break;
		}
	}
	if(has_odd){
		memcpy(_state, _seed, 6*12);
		((uint8_t*)_state)[6*12] = !!((uint64_t*)_seed)[6*12];
	}
	return has_odd;
}

cr8r_prng *cr8r_prng_init_lfg(const char seed[static 6*12 + 1]){
	cr8r_prng *res = malloc(offsetof(cr8r_prng, state) + 6*12 + 1);
	if(res){
		if(!cr8r_prng_lfg_seed(res->state, seed)){
			free(res);
			return NULL;
		}
		res->state_size = 6*12 + 1;
		res->get_u32 = cr8r_prng_lfg_get_u32,
		res->seed = cr8r_prng_lfg_seed;
	}
	return res;
}


// the Mersenne twister implementation is tightly based on wikipedia
// (https://en.wikipedia.org/wiki/Mersenne_Twister)
// and uses the standard MT19937-64 generator configuration
// with the modification that each 64 bit output is split into 2 32 bit
// outputs to conform with the cr8r_prng interface.
// The 32 bit generator configuration MT19937 could have been used instead.
// Both have exactly the same state size and generate the same number
// of random bits before needing to call "twist".
// MT19937 will probably replace this hybrid approach, and parameterized
// versions of LCG, LFG, and MT will probably be added.
typedef struct{
	uint64_t index;
	uint64_t MT[CR8R_PRNG_MT_N];
} cr8r_prng_mt_st;

static const uint64_t cr8r_prng_mt_lomask = (1ull << CR8R_PRNG_MT_R) - 1;
static const uint64_t cr8r_prng_mt_himask = ~cr8r_prng_mt_lomask;
 
static bool cr8r_prng_mt_seed(void *_state, const void *_seed){
	uint64_t seed = *(const uint64_t*)_seed;
	if(!seed){
		return false;
	}
	cr8r_prng_mt_st *state = _state;
	state->index = 2*CR8R_PRNG_MT_N;
	state->MT[0] = seed;
	for(uint64_t i = 1; i < CR8R_PRNG_MT_N; ++i){
		state->MT[i] = CR8R_PRNG_MT_F*(state->MT[i-1]^(state->MT[i-1] >> 62)) + i;
	}
	return true;
}

uint32_t cr8r_prng_mt_get_u32(void *_state){
	cr8r_prng_mt_st *state = _state;
	if(state->index >= 2*CR8R_PRNG_MT_N){
		for(uint64_t i = 0; i < CR8R_PRNG_MT_N; ++i){
			uint64_t x = (state->MT[i]&cr8r_prng_mt_himask)
				| (state->MT[(i+1)%CR8R_PRNG_MT_N]&cr8r_prng_mt_lomask);
			uint64_t xA = x >> 1;
			if(x&1){
				xA ^= CR8R_PRNG_MT_A;
			}
			state->MT[i] = state->MT[(i + CR8R_PRNG_MT_M)%CR8R_PRNG_MT_N]^xA;
		}
		state->index = 0;
	}
	// we split each 64 bit output into 2 32 bit outputs,
	// but both are computed from the state which is a little redundant
	uint64_t y = state->MT[state->index >> 1];
	y ^= (y >> CR8R_PRNG_MT_U)&CR8R_PRNG_MT_D;
	y ^= (y << CR8R_PRNG_MT_S)&CR8R_PRNG_MT_B;
	y ^= (y << CR8R_PRNG_MT_T)&CR8R_PRNG_MT_C;
	y ^= (y >> CR8R_PRNG_MT_L);
	if(state->index++&1){
		return y >> 32;
	}
	return y;
}

cr8r_prng *cr8r_prng_init_mt(uint64_t seed){
	cr8r_prng *res = malloc(offsetof(cr8r_prng, state) + sizeof(cr8r_prng_mt_st));
	if(res){
		if(!cr8r_prng_mt_seed(res->state, &seed)){
			free(res);
			return NULL;
		}
		res->state_size = sizeof(cr8r_prng_mt_st);
		res->get_u32 = cr8r_prng_mt_get_u32,
		res->seed = cr8r_prng_mt_seed;
	}
	return res;
}

// The xoroshift256** by David Blackman and Sebastiano Vigna
// is available in its original form at https://vigna.di.unimi.it/xorshift/
// The original is released into the public domain with the following
// notice, and this adapted version is also public domain

// Original notice:

/*  Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/* This is xoshiro256** 1.0, one of our all-purpose, rock-solid
   generators. It has excellent (sub-ns) speed, a state (256 bits) that is
   large enough for any parallel application, and it passes all tests we
   are aware of.

   For generating just floating-point numbers, xoshiro256+ is even faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */

static inline uint64_t rotl(uint64_t x, uint64_t k){
	return (x << k) | (x >> (64 - k));
}

static uint32_t cr8r_prng_xoro_get_u32(void *_state){
	uint64_t *s = _state;
	uint64_t res = rotl(s[1]*5, 7)*9;
	uint64_t t = s[1] << 17;
	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];
	s[2] ^= t;
	s[3] = rotl(s[3], 45);
	return res >> 16;
}

void cr8r_prng_xoro_jump_t128(cr8r_prng *self){
	uint64_t *s = (uint64_t*)self->state;
	static const uint64_t JUMP[] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c};
	uint64_t s0 = 0;
	uint64_t s1 = 0;
	uint64_t s2 = 0;
	uint64_t s3 = 0;
	for(uint64_t i = 0; i < 4; ++i){
		for(uint64_t b = 0; b < 64; ++b){
			if(JUMP[i]&(1ull << b)){
				s0 ^= s[0];
				s1 ^= s[1];
				s2 ^= s[2];
				s3 ^= s[3];
			}
			cr8r_prng_xoro_get_u32(self->state);
		}
	}
	s[0] = s0;
	s[1] = s1;
	s[2] = s2;
	s[3] = s3;
}

void cr8r_prng_xoro_jump_t192(cr8r_prng *self){
	uint64_t *s = (uint64_t*)self->state;
	static const uint64_t LONG_JUMP[] = { 0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635 };
	uint64_t s0 = 0;
	uint64_t s1 = 0;
	uint64_t s2 = 0;
	uint64_t s3 = 0;
	for(uint64_t i = 0; i < 4; ++i){
		for(uint64_t b = 0; b < 64; ++b){
			if(LONG_JUMP[i]&(1ull << b)){
				s0 ^= s[0];
				s1 ^= s[1];
				s2 ^= s[2];
				s3 ^= s[3];
			}
			cr8r_prng_xoro_get_u32(self->state);
		}
	}
	s[0] = s0;
	s[1] = s1;
	s[2] = s2;
	s[3] = s3;
}

static bool cr8r_prng_xoro_seed(void *_state, const void *_seed){
	memcpy(_state, _seed, 32);
	return true;//TODO: consider checking the input for quality?
}

cr8r_prng *cr8r_prng_init_xoro(const char seed[static 32]){
	cr8r_prng *res = malloc(offsetof(cr8r_prng, state) + 32);
	if(res){
		if(!cr8r_prng_xoro_seed(res->state, seed)){
			free(res);
			return NULL;
		}
		res->state_size = 32;
		res->get_u32 = cr8r_prng_xoro_get_u32,
		res->seed = cr8r_prng_xoro_seed;
	}
	return res;
}

// End of xoroshift256** code

