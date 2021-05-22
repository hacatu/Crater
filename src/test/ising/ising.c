#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <stddef.h>

#include "ising.h"

void ising2D_step(ising2D_lattice *self, cr8r_prng *prng){
	uint64_t N = self->size*self->size;
	uint64_t i = cr8r_prng_uniform_u64(prng, 0, N);
	uint_fast8_t neighbor_mask = 0;
	const uint64_t offsets[5] = {1, N - 1, self->size, N - self->size, 0};
	for(uint64_t oi = 0; oi < 5; ++oi){
		uint64_t j = (i + offsets[oi])%N;
		neighbor_mask <<= 1;
		neighbor_mask |= !!(self->spin_masks[j >> 6] & (1ull << (j&0x3F)));
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
	int64_t num_not_matching = __builtin_popcount((-(neighbor_mask & 1)^neighbor_mask) & 0x1E);
	if(num_not_matching >= 2 || cr8r_prng_uniform01_double(prng) <= exp(self->B*(4*num_not_matching - 8))){
		self->spin_masks[i >> 6] ^= 1ull << (i&0x3F);
	}
}

// to compute the distribution of some function of the lattice state (eg the energy per site),
// we can use grey code to switch through all states optimally and hopefully compute the function
// simply by knowing it on the homogeneous spin up state and knowing how changing a single site
// affects it.  This works for energy, in particular the energy of a homogeneous state is 2*N
// where N is the number of sites in the (square, toroidal) lattice.

int64_t ising2D_flip(ising2D_lattice *self, uint64_t i){
	uint64_t N = self->size*self->size;
	uint_fast8_t neighbor_mask = 0;
	const uint64_t offsets[5] = {1, N - 1, self->size, N - self->size, 0};
	for(uint64_t oi = 0; oi < 5; ++oi){
		uint64_t j = (i + offsets[oi])%N;
		neighbor_mask <<= 1;
		neighbor_mask |= !!(self->spin_masks[j >> 6] & (1ull << (j&0x3F)));
	}
	self->spin_masks[i >> 6] ^= 1ull << (i&0x3F);
	int64_t num_not_matching = __builtin_popcount((-(neighbor_mask & 1)^neighbor_mask) & 0x1E);
	//fprintf(stderr, "\e[1;35mneighbor_mask=%"PRIuFAST8", num_not_matching=%"PRIi64"\e[0m\n", neighbor_mask, num_not_matching);
	return 4*num_not_matching - 8;
}

bool ising2D_compute_energy_probs(uint64_t n, double B, double energy_probs[static n*n + 1]){
	int64_t N = n*n;
	if(N > 63){
		return false;
	}
	ising2D_lattice *latt = calloc(offsetof(ising2D_lattice, spin_masks) + sizeof(uint64_t), 1);
	if(!latt){
		return false;
	}
	latt->size = n;
	latt->B = B;
	memset(energy_probs, 0, N*sizeof(double));
	int64_t energy = 2*N;
	energy_probs[N] = exp(B*energy);
	for(uint64_t i = 1, prev_gray = 0, curr_gray; i < (1ull << N); ++i){
		curr_gray = i^(i >> 1);
		/*
		for(uint64_t j = 0; j < n; ++j){
			fprintf(stderr, "\e[1;33m(");
			if(!j){
				fprintf(stderr, "\e[53m");//overline
			}else if(j + 1 == n){
				fprintf(stderr, "\e[4m");//underline
			}
			for(uint64_t i = 0; i < n; ++i){
				uint64_t k = j*3 + i;
				fprintf(stderr, "%"PRIu64, !!(latt->spin_masks[i >> 6] & (1ull << (k&0x3F))));
			}
			fprintf(stderr, "\e[24;55m)\e[0m\n");//unset over/underline before ")"
		}
		*/
		uint64_t j = __builtin_ctzll(curr_gray^prev_gray);
		int64_t energy_change = ising2D_flip(latt, __builtin_ctzll(curr_gray^prev_gray));
		//fprintf(stderr, "\e[1;33m  --> H=%"PRIi64", flipping %"PRIu64", dH=%"PRIi64"\e[0m\n", energy, j, energy_change);
		
		energy += energy_change;
		// max energy is 2*N, min step is 4, and min energy is >= -2*N, so there are
		// at most N + 1 energy levels.  We can subtract the number of levels below 2*N we are,
		// that is, N is the index for 2*N and in general N - (2*N - energy)/4
		// = (4*N - 2*N + energy)/4 = (2*N + energy)/4
		energy_probs[(2*N + energy)/4] += exp(B*energy);
		prev_gray = curr_gray;
	}
	free(latt);
	double Z = 0;
	for(uint64_t i = 0; i <= (uint64_t)N; ++i){
		Z += energy_probs[i];
	}
	for(uint64_t i = 0; i <= (uint64_t)N; ++i){
		energy_probs[i] /= Z;
	}
	return true;
}

// the ising model is the simplest model in statistical mechanics that has a critical point.
// the model can be thought of as a grid of point charges of charge +/-1, only considering the interactions
// between each particle and its neighbors.  that is, each pair of adjacent equal charges decreases the
// total potential energy of the configuration, and each pair of adjacent unequal charges increases the
// total potential energy.  Given an inverse temperature B, we can associate a probability with each state
// based only on its energy.  B = log(1 + sqrt(2))/2 is a critical point

