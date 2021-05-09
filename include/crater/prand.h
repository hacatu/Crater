#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// @section LICENSE
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.
/// @section DESCRIPTION
/// Comprehensive pseudorandom number generation interfaces.
///
/// PRNGs in general function by having an internal state (8-2496
/// bytes for the currently implemented generators), applying
/// a function to the whole state, and then returning a part of the
/// state (always 4 bytes for the generators implemented here).
/// The function that transforms the state is not necessarily difficult
/// to reverse or predict.
///
/// However, the entire state should be impossible to recover from
/// less than state/4 outputs and difficult to recover from state_size/4 outputs.
/// More importantly, the outputs should satisfy certain randomness
/// tests, like producing bytes with equal frequency and so forth.
/// Some of the most well known and widely used tests are the diehard
/// tests, testU01, and the NIST tests.
///
/// PRNGs need to have their state initialized by "seeding".
/// This can be done either by setting the entire state or by using a PRNG with
/// smaller state to generate more state from a smaller initial state.
/// For example, the Mersenne Twister has 2496 bytes of state but is usually
/// initialized from 8 bytes using a Linear Congruential Generator.
/// Whether the full state is needed or a smaller state is automatically expanded
/// is specified in the documentation for the specific generator types.
/// The quality of some PRNGs is more sensitive to the seed than that of others.
///
/// PRNGs generally have a finite period, meaning they eventually repeat.
/// You can generally expect this to be around at most 2^B where B is the number
/// of state bits.  For instance, an LCG with modulus 2**64 has maximal period
/// 2**62, the standard MT with 19968 bits of state has period 2**19967, and
/// Xoroshiro256** with 256 bits of state has period 2**256 (maximal for any
/// possible PRNG by the pidgeonhole principle).
///
/// You may wish to write a program that generates random numbers in multiple
/// threads.  Generally, Crater functions require you to do appropriate locking
/// on your own, but for PRNGs the situation is more subtle.
/// If using a single PRNG from multiple threads, it MUST be locked, unless it
/// is known to be a "system" PRNG since then the state is maintained by the OS.
/// However, locking is usually not ideal because it has significant overhead if
/// you are generating a lot of random numbers.  Therefore, use a different
/// PRNG in each thread.  HOWEVER, you should NOT simply use a different seed for
/// each, as for some PRNG types this can cause increased correlation.  Instead,
/// the particular "jump" functions should be used to split up the output of the
/// PRNG for a single seed.  "jump" functions allow a PRNG to be advanced as if it
/// were called a huge number of times very quickly.  Currently, only 2 fixed
/// size jump functions for Xoro are available, but more will be added soon.
///
/// Finally, some PRNGs are more suitable for cryptography and secure
/// purporses than others.  Generally this comes at the cost of speed.
/// Only "system", the wrapper around Linux's getrandom syscall,
/// should be considered secure.

#include <inttypes.h>
#include <stdbool.h>

/// A PseudoRandom Number Generator
///
/// This is a generic representation, generally to be heap-allocated
/// by one of the functions in this file.
/// That means the prng state is stored in the flexible length array
/// in the same allocation, decreasing memory fragmentation and
/// making copying prng state easier.
typedef struct{
	/// Size of the state information for the implementation, in bytes
	uint64_t state_size;
	/// Callback to get 4 unsigned bytes from the underlying generator
	uint32_t (*get_u32)(void*);
	/// Callback to seed the underlying generator.
	/// This can either mean "seed with enough entropy to populate the
	/// entire state" or "seed with a small <= 8 byte value, extended
	/// to the full state size with some PRNG".
	bool (*seed)(void*, const void*);
	/// flexible length array so that the generator's state can be stored in the same allocation
	char state[];
} cr8r_prng;

/// Set the state of a prng
///
/// See the generator initialization functions for information on whether a single uint64_t
/// or the entire state should be supplied, as well as what inputs are disallowed.
/// @return true if the seed value is acceptable for the given generator, false otherwise
bool cr8r_prng_seed(cr8r_prng*, const char seed[*]);

/// Get a single uint32_t from a prng
///
/// Some prngs have a different natural output size than 32 bits, but all have been coerced into
/// a size of 32 bits.  This is because a lot of the time 32 bits is more than enough and 64 bits
/// would require 2 calls for LCGs and LFGs, and MTs and Xoroshift can have problems with entropy in
/// low bits anyway.
uint32_t cr8r_prng_get_u32(cr8r_prng*);

/// Get a single uint64_t from a prng
///
/// Always results in 2 calls to self->get_u32 in the current implementation
uint64_t cr8r_prng_get_u64(cr8r_prng*);

/// Fill a buffer with random bytes from a prng
///
/// Gotten from ceil(size / 4) calls to self->get_u32
void cr8r_prng_get_bytes(cr8r_prng*, uint64_t size, char buf[static size]);

/// Get a uint64_t which is uniformly distributed on [a, b).
///
/// The output is not low-biased: if b - a <= 1 << 32, the generator
/// is sampled 32 bits at a time until a result below the highest multiple
/// of b - a <= 1 << 32 is obtained.  This should always take less than 2 samples
/// on average.  If b - a > 1 << 32, the generator is sampled 64 bits at a
/// time until a result below the highest multiple of b - a <= 1 << 64 is obtained.
/// This should always take less than 4 32 bit samples on average.
uint64_t cr8r_prng_uniform_u64(cr8r_prng*, uint64_t a, uint64_t b);

// these constants are taken from the standard MT19937-64 generator configuration
#define CR8R_PRNG_MT_N 312
#define CR8R_PRNG_MT_M 156
#define CR8R_PRNG_MT_R 31
#define CR8R_PRNG_MT_A 0xB5026F5AA96619E9
#define CR8R_PRNG_MT_U 29
#define CR8R_PRNG_MT_D 0x5555555555555555
#define CR8R_PRNG_MT_S 17
#define CR8R_PRNG_MT_B 0x71D67FFFEDA60000
#define CR8R_PRNG_MT_T 37
#define CR8R_PRNG_MT_C 0xFFF7EEE000000000
#define CR8R_PRNG_MT_L 43
#define CR8R_PRNG_MT_F 6364136223846793005

/// Create a PRNG based on the system's prng device
///
/// On Linux, this is based on "getrandom".
/// The state size is 0 and attempts to seed generators of
/// this type fail.
/// @return pointer to new system based prng (must be free'd), or NULL on failure (allocation/unsupported)
cr8r_prng *cr8r_prng_init_system();

/// Create a PRNG based on a Linear Congruential Generator
///
/// Seed value must not be 0!
/// Seed value is a single uint64_t, on calls to { @link cr8r_prng_seed } it should be
/// passed by address.
/// LCGs are extremely fast and simple PRNGs that work by the rule
/// x_n = a*x_(n-1) + c mod m.
/// So if m is a power of 2 larger than 4 (ie 8 or higher),
/// the maximal period is m/4 which is achieved for any a = 3 or 5 mod 8.
/// The LCG parameters used are the same as muslc, namely
/// a = 6364136223846793005, c = 1, and m = 2**64.  Using any other values for m and c
/// is unreasonable.  The state is simply a 64 bit number, and the hi 32 bits are returned
/// for each output.
/// @return pointer to new lcg based prng (must be free'd), or NULL on failure (allocation/seed == 0)
/// ( { @link cr8r_prng_seed } will also fail if seed == 0 is passed)
cr8r_prng *cr8r_prng_init_lcg(uint64_t seed);

/// Create a PRNG based on a Lagged Fibonacci Generator
///
/// Seed value is used as a sequence of 12 48-bit integers followed by
/// a single bit.  At least one term in the seed sequence must be odd!
/// In general, an LFG is defined by two "lookbacks" and a binary operation,
/// so that x_n = x_[n-s] # x_[n-r] mod m for some binary operation #.
/// This implementation uses the same parameters (namely s == 12, r == 5,
/// m == 2**48, # == subtraction with carry) as the C++ standard library.
/// The extra state/seed bit is the carry bit.  In general,
/// choosing good s nd r values is possible by considering irreducible polynomials
/// of the form x^s + x^r + 1 mod 2 with s/r approaching the golden ratio.
/// However, choosing the seed value is difficult and I could not find a good
/// reference for it.
/// @return pointer to new lfg based prng (must be free'd), or NULL on failure
/// (allocation/seed has no odd terms) ( { @link cr8r_prng_seed }
/// will also fail if seed == 0 is passed)
cr8r_prng *cr8r_prng_init_lfg(const char seed[static 6*12 + 1]);

/// Create a PRNG based on a Mersenne Twister Generator
///
/// Seed value is a single uint64_t.  If calling { @link cr8r_prng_seed } it should be
/// passed by pointer.  The state is 312 uint64_t's (2496 bytes), plus an additional
/// index (1 more uint64_t or 8 more bytes).  The full state is generated from the seed
/// by a LCG. The parameters used here (including for the LCG) are
/// the standard MT19937-64 generator configuration.
/// @return pointer to new MT based prng (must be free'd), or NULL on failure
/// (allocation; currently all seed values are permitted)
cr8r_prng *cr8r_prng_init_mt(uint64_t seed);

/// Create a PRNG based on Vigna and Blackman's Xoroshiro256** algorithm
///
/// This PRNG seems to be very fast, closer to an LCG than a bulkier but more typical
/// feedback shift register algorithm like MT, yet still have good properties on most tests.
/// However, it has not been widely used for as long as MT.
/// The seed/state is 32 bytes (4 uint64_t's)
/// Also see { @link cr8r_prng_xoro_jump_t128 } and { @link cr8r_prng_xoro_jump_t192 }
/// @return pointer to new xoroshiro256** based prng (must be free'd), or NULL on failure
/// (allocation; currently seed value is not quality checked)
cr8r_prng *cr8r_prng_init_xoro(const char seed[static 32]);

/// Jump a xoro based prng forwards by 2**128 steps quickly
///
/// Do not call on non xoro prngs, it will scramble your memory.
/// Remember the period of xoroshiro256** is 2**256.
void cr8r_prng_xoro_jump_t128(cr8r_prng*);

/// Jump a xoro based prng forwards by 2**192 steps quickly
///
/// Do not call on non xoro prngs, it will scramble your memory.
/// Remember the period of xoroshiro256** is 2**256.
void cr8r_prng_xoro_jump_t192(cr8r_prng*);

