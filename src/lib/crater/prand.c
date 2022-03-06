#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>

#include <crater/prand.h>

typedef union{
	double as_double;
	struct{
		uint64_t fraction:52, exp:11, sign:1;
	};
} double_bits_t;

bool cr8r_prng_seed(cr8r_prng *self, uint64_t seed){
	if(self->state_size > sizeof(uint64_t)){
		uint64_t _state = *(uint64_t*)cr8r_default_prng_splitmix->state;
		*(uint64_t*)cr8r_default_prng_splitmix->state = seed;
		cr8r_prng_get_bytes(cr8r_default_prng_splitmix, self->state_size, self->state);
		*(uint64_t*)cr8r_default_prng_splitmix->state = _state;
	}else{
		memcpy(self->state, &seed, self->state_size);
	}
	return self->fixup_state(self->state);
}

uint32_t cr8r_prng_get_u32(cr8r_prng *self){
	return self->get_u32(self->state);
}

uint64_t cr8r_prng_get_u64(cr8r_prng *self){
	uint64_t res = self->get_u32(self->state);
	return res | ((uint64_t)self->get_u32(self->state) << 32);
}

void cr8r_prng_get_bytes(cr8r_prng *self, uint64_t size, void *buf){
	uint64_t i;
	for(i = 0; i + sizeof(uint32_t) <= size; i += sizeof(uint32_t)){
		uint32_t tmp = self->get_u32(self->state);
		memcpy(buf + i, &tmp, sizeof(uint32_t));
	}
	if(i != size){
		uint32_t tmp = self->get_u32(self->state);
		memcpy(buf + i, &tmp, size - i);
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

double cr8r_prng_uniform01_double(cr8r_prng *self){
	uint64_t u = cr8r_prng_get_u64(self);
	if(!u){
		return 0.;
	}
	uint64_t exp_decrease = __builtin_clzll(u);
	u <<= exp_decrease;
	double_bits_t bits = {.sign= 0, .exp= 1022 - exp_decrease, .fraction= u >> 12};
	return bits.as_double;
}

const uint64_t cr8r_prng_2tg_t64[4] = {1, (1ull << 63) + 3, (1ull << 63) - 1, -1};

// raise b to the power of 2**i mod 2**j
inline static uint64_t pow_ti(uint64_t b, uint64_t i){
	while(i--){
		b *= b;
	}
	return b;
}

uint64_t cr8r_prng_log_mod_t64(uint64_t h){
	uint64_t y = pow_ti(3, 61);
	uint64_t g1 = 12297829382473034411ull;
	for(uint64_t gi = 0; gi < 4; ++gi){
		uint64_t x = 0;
		uint64_t b = h*cr8r_prng_2tg_t64[gi];
		// maintain g1_ti = g1**(2**i)
		uint64_t g1_tk = g1;
		uint64_t k = 0;
		for(; k < 62; ++k){
			uint64_t hk = pow_ti(b, 61 - k);
			if(hk == 1){
				// x is not modified
			}else if(hk == y){
				x |= 1ull << k;
				b *= g1_tk;
			}else{
				break;
			}
			g1_tk = g1_tk*g1_tk;
		}
		if(k == 62){
			// x was successfully extended to 62 bits, set the high 2 bits based on what
			// the first generator that worked was (0b00 for 3, 0b01 for -3,
			// 0b10 for 2**(k-1)+3, or 0b11 for 2**(k-1)-3)
			return (gi << 62) |  (x&((1ull << 62) - 1));
		}
	}
	// reachable iff h is even
	return 0;
}

static uint32_t cr8r_prng_system_get_u32(void *_state){
	uint32_t res;
	getrandom(&res, sizeof(uint32_t), 0);
	return res;
}

static bool cr8r_prng_system_fixup(void *_state){
	return true;
}

cr8r_prng *cr8r_prng_init_system(){
	cr8r_prng *res = malloc(offsetof(cr8r_prng, state));
	if(res){
		res->state_size = 0;
		res->get_u32 = cr8r_prng_system_get_u32;
		res->fixup_state = cr8r_prng_system_fixup;
	}
	return res;
}

static uint32_t cr8r_prng_lcg_get_u32(void *_state){
	uint64_t *state = _state;
	*state = 6364136223846793005*(*state) + 1;
	return *state >> 32;
}

static bool cr8r_prng_lcg_fixup(void *_state){
	uint64_t *state = _state;
	if(!*state){
		*state = CR8R_DEFAULT_PRNG_LCG_SEED;
	}
	return true;
}

cr8r_prng *cr8r_prng_init_lcg(uint64_t seed){
	cr8r_prng *res = malloc(offsetof(cr8r_prng, state) + sizeof(uint64_t));
	if(res){
		res->state_size = sizeof(uint64_t);
		res->get_u32 = cr8r_prng_lcg_get_u32;
		res->fixup_state = cr8r_prng_lcg_fixup;
		if(!cr8r_prng_seed(res, seed)){
			free(res);
			return NULL;
		}
	}
	return res;
}

static uint32_t cr8r_prng_lfg_sc_get_u32(void *_state){
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

static bool cr8r_prng_lfg_sc_fixup(void *_state){
	char *state = _state;
	for(uint64_t i = 0; i < 12; i += 6){
		// depends on little endianness
		if(state[i]&1){
			return true;
		}
	}
	for(uint64_t i = 0; i < 12; i += 6){
		state[i] |= 1;
	}
	return true;
}

cr8r_prng *cr8r_prng_init_lfg_sc(uint64_t seed){
	cr8r_prng *res = malloc(offsetof(cr8r_prng, state) + 6*12 + 1);
	if(res){
		res->state_size = 6*12 + 1;
		res->get_u32 = cr8r_prng_lfg_sc_get_u32,
		res->fixup_state = cr8r_prng_lfg_sc_fixup;
		if(!cr8r_prng_seed(res, seed)){
			free(res);
			return NULL;
		}
	}
	return res;
}

typedef struct{
	uint64_t index;
	uint64_t XS[CR8R_PRNG_LFM_R];
} cr8r_prng_lfg_m_state;

static uint32_t cr8r_prng_lfg_m_get_u32(void *_state){
	cr8r_prng_lfg_m_state *state = _state;
	uint64_t next_index = state->index ? state->index - 1 : CR8R_PRNG_LFM_R - 1;
	state->XS[next_index] *= state->XS[(state->index + CR8R_PRNG_LFM_S - 1)%CR8R_PRNG_LFM_R];
	state->index = next_index;
	return state->XS[next_index] >> 16;
}

static bool cr8r_prng_lfg_m_fixup(void *_state){
	cr8r_prng_lfg_m_state *state = _state;
	for(uint64_t i = 0; i < CR8R_PRNG_LFM_R; ++i){
		state->XS[i] |= 1;
	}
	state->index %= CR8R_PRNG_LFM_R;
	return true;
}

cr8r_prng *cr8r_prng_init_lfg_m(uint64_t seed){
	cr8r_prng *res = malloc(offsetof(cr8r_prng, state) + sizeof(cr8r_prng_lfg_m_state));
	if(res){
		res->state_size = sizeof(cr8r_prng_lfg_m_state);
		res->get_u32 = cr8r_prng_lfg_m_get_u32;
		res->fixup_state = cr8r_prng_lfg_m_fixup;
		if(!cr8r_prng_seed(res, seed)){
			free(res);
			return NULL;
		}
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

/*
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
*/

static bool cr8r_prng_mt_fixup(void *_state){
	return true;
}

static uint32_t cr8r_prng_mt_get_u32(void *_state){
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
		res->state_size = sizeof(cr8r_prng_mt_st);
		res->get_u32 = cr8r_prng_mt_get_u32,
		res->fixup_state = cr8r_prng_mt_fixup;
		if(!cr8r_prng_seed(res, seed)){
			free(res);
			return NULL;
		}
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
	static const uint64_t LONG_JUMP[] = {0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635};
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

static bool cr8r_prng_xoro_fixup(void *_state){
	return true;
}

cr8r_prng *cr8r_prng_init_xoro(uint64_t seed){
	cr8r_prng *res = malloc(offsetof(cr8r_prng, state) + 4*sizeof(uint64_t));
	if(res){
		res->state_size = 4*sizeof(uint64_t);
		res->get_u32 = cr8r_prng_xoro_get_u32,
		res->fixup_state = cr8r_prng_xoro_fixup;
		if(!cr8r_prng_seed(res, seed)){
			free(res);
			return NULL;
		}
	}
	return res;
}

/*  Written in 2015 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/* This is a fixed-increment version of Java 8's SplittableRandom generator
   See http://dx.doi.org/10.1145/2714064.2660195 and 
   http://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html

   It is a very fast generator passing BigCrush, and it can be useful if
   for some reason you absolutely want 64 bits of state. */

static uint32_t cr8r_prng_splitmix_get_u32(void *_state){
	uint64_t *state = _state;
	uint64_t z = (*state += 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	z ^= (z >> 31);
	return z >> 16;
}

static bool cr8r_prng_splitmix_fixup(void *_state){
	return true;
}

cr8r_prng *cr8r_prng_init_splitmix(uint64_t seed){
	cr8r_prng *res = malloc(offsetof(cr8r_prng, state) + sizeof(uint64_t));
	if(res){
		res->state_size = sizeof(uint64_t);
		res->get_u32 = cr8r_prng_splitmix_get_u32,
		res->fixup_state = cr8r_prng_splitmix_fixup;
		if(!cr8r_prng_seed(res, seed)){
			free(res);
			return NULL;
		}
	}
	return res;
}

static struct{
	uint64_t state_size;
	uint32_t (*get_u32)(void*);
	bool (*fixup_state)(void*);
	uint64_t state;
} _default_prng_splitmix = {
	.state_size = sizeof(uint64_t),
	.get_u32 = cr8r_prng_splitmix_get_u32,
	.fixup_state = cr8r_prng_splitmix_fixup,
	.state = CR8R_DEFAULT_PRNG_SM_SEED
};
cr8r_prng *cr8r_default_prng_splitmix = (cr8r_prng*)&_default_prng_splitmix;

// End of xoroshift256** code

