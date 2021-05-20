#pragma once

#include <inttypes.h>

#include <crater/prng.h>

typedef struct{
	uint64_t size;
	double B;
	uint64_t spin_masks[];
} ising2D_lattice;

void ising2D_step(ising2D_lattice *self, cr8r_prng *prng);

int64_t ising2D_flip(ising2D_lattice *self, uint64_t i);

bool ising2D_compute_energy_probs(uint64_t n, double B, double energy_probs[static n*n + 1]);

