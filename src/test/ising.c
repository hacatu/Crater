#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include <crater/prand.h>

typedef struct{
	uint64_t size;
	double B;
	uint64_t spin_masks[];
} ising2D_lattice;

void ising2D_step(ising2D_lattice *self, cr8r_prng *prng){
	uint64_t N = self->size*self->size;
	uint64_t i = cr8r_prng_uniform_u64(prng, 0, N);
	uint_fast8_t neighbor_mask = 0;
	const uint64_t offsets[5] = {1, N - 1, self->size, N - self->size, 0};
	for(uint64_t oi = 0; oi < 5; ++oi){
		uint64_t j = i + offsets[oi];
		neighbor_mask |= !!(self->spin_masks[j >> 6] & (1ull << (j&0x3F)));
		neighbor_mask <<= 1;
	}
	// the energy contribution for the current state for lattice point i
	// is the number of bits in neighbor_mask & 0x1D not matching neighbor_mask & 1,
	// minus the number matching, or 2*(number not matching) - 4.
	// also notice that the energy contribution for the current state and the state with
	// lattice point i flipped sum to zero.  thus, the energy increases when flipping if
	// 2*(number not matching) < 4 ---> (number not matching) < 2, which we can compute
	// as
	// number not matching = __builtin_popcount((-(neighbor_mask & 1)^neighbor_mask) & 0x1D) < 2.
	// and the increase in energy will be 8 - 4*(number not matching)
	uint64_t num_not_matching = __builtin_popcount((-(neighbor_mask & 1)^neighbor_mask) & 0x1D);
	if(num_not_matching >= 2 || cr8r_prng_uniform01_double(prng) <= exp(-self->B*(8 - 4*num_not_matching))){
		self->spin_masks[i >> 6] ^= 1ull << (i&0x3F);
	}
}

// the ising model is the simplest model in statistical mechanics that has a critical point.
// the model can be thought of as a grid of point charges of charge +/-1, only considering the interactions
// between each particle and its neighbors.  that is, each pair of adjacent equal charges decreases the
// total potential energy of the configuration, and each pair of adjacent unequal charges increases the
// total potential energy.  Given an inverse temperature B, we can associate a probability with each state
// based only on its energy.  B = log(1 + sqrt(2))/2 is a critical point

int main(){
	// not yet implemented
}

