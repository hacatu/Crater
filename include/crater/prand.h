#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
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
/// The cr8r_prng_init_* functions use a uint32_t as a seed by default.
/// This is generally sufficient, but keep in mind the disadantages and consider
/// if setting the full state is required for your use case.
///
/// A prng's output sequence is a mathematical function of its state.  Thus,
/// when a prng is initialized from a 32 bit seed, there are only 2**32 possible
/// output sequences (or fewer if the state has some restriction).
/// This is usually a lot, but can skew probabilities in some cases.
/// For example, for some prngs it is impossible to generate 0 as the first output
/// if initial state is obtained by extending a 32 bit seed.
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
/// The fact that randomized seeds are (usually, not as much for lcgs) worse than
/// "jumping" to equidistant points in the output sequence may be very
/// counterintuitive.  It was for me at first.  The reason is due to an even
/// deeper peculiarity of prngs: namely, a maximal period prng goes through each
/// STATE at most once per period by the pidgeonhole principle.  But moreover,
/// the k-tuples it produces are limited: for large enough k, it is mathematically
/// impossible for it to produce every possible k-tuple.  For example, if the
/// state is 128 bytes, there are at most 2**1024 states.  However, there are
/// 2**1032 129-tuples of bytes, so it is impossible for such a prng to generate all
/// such tuples.  We want to generate all k-tuples for as large a k as possible
/// without compromising randomness though.  To achieve this, it is common for
/// prngs to generate all k-tuples equally often for some k smaller than their state,
/// For k == 1, this represents the simple fact that all bytes should be generated
/// equally likely.  True random bytes are equally distributed.  And while true random
/// k-tuples are equally likely, the probability of generating a fixed number of
/// truly random k-tuples and getting perfectly equal amounts of all of them
/// goes to zero very quickly.  So in this sense the output of a prng can be very contrived,
/// but this is necessary to make as many sequences as possible occur in the output.
///
/// Essentially, this is a tradeoff between "local" and "global" properties of the output
/// sequence.  An experement that requires random numbers cares about these local properties
/// (is every sufficiently short byte sequence possible and equally likely, are outputs
/// uncorrelated, etc), but not global properties (do all k-tuples occur exactly the same
/// number of times).  Put another way, you would expect the ratio of frequencies of any
/// two k-tuples to approach 1 as you generated more and more random numbers, but
/// you would expect the absolute difference to possibly be very large.
///
/// Finally, some PRNGs are more suitable for cryptography and secure
/// purporses than others.  Generally this comes at the cost of speed.
/// Only "system", the wrapper around Linux's getrandom syscall,
/// should be considered secure.
///
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
	/// Called after "randomizing" the state.
	/// Typically the state is initialized using splitmix and then this function is called to
	/// ensure it is good.
	bool (*fixup_state)(void*);
	/// flexible length array so that the generator's state can be stored in the same allocation
	char state[];
} cr8r_prng;

/// Set the state of a prng
///
/// WARNING: accesses { @link cr8r_default_prng_splitmix } for generators where state_size > 4,
/// all accesses to the default splitmix prng should be done in the same thread or protected by locks.
/// If necessary, the given seed is extended using splitmix to generate random bytes according to
/// prng->state_size, and then prng->fixup_state is called.  Returns false only if fixup_state
/// fails.
bool cr8r_prng_seed(cr8r_prng*, uint64_t);

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
void cr8r_prng_get_bytes(cr8r_prng*, uint64_t size, void *buf);

/// Get a uint64_t which is uniformly distributed on [a, b).
///
/// The output is not low-biased: if b - a <= 1 << 32, the generator
/// is sampled 32 bits at a time until a result below the highest multiple
/// of b - a <= 1 << 32 is obtained.  This should always take less than 2 samples
/// on average.  If b - a > 1 << 32, the generator is sampled 64 bits at a
/// time until a result below the highest multiple of b - a <= 1 << 64 is obtained.
/// This should always take less than 4 32 bit samples on average.
uint64_t cr8r_prng_uniform_u64(cr8r_prng*, uint64_t a, uint64_t b);

/// Get a double which is uniformly distributed on [0, 1)
///
/// Does not include denormalized numbers, only normal format doubles
/// from 0 inclusive to 1 exclusive.
double cr8r_prng_uniform01_double(cr8r_prng*);

/// Find x (the discrete logarithm) so that h = g**x mod 2**64
///
/// This is a key step in "jumping" multiplicative lagged Fibonacci generators
/// because it allows us to rewrite x_i = x_(i-s) * x_(i-r) mod 2**k
/// by expressing x_i as g**(a_i mod 2**(k-2)).
///
/// At least, it would let us do this, but discrete logarithm modulo powers of 2
/// works differently than discrete logarithm modulo powers of odd primes.
/// A generator for the multiplicative group only exists mod 2, 4, p**k, and 2*p**k where
/// p is any odd prime.  In particular, the multiplicative group modulo 2**k does not
/// have a generator for k > 2 and instead decomposes as the direct product of the multiplicative
/// group mod 8 and the cyclic group mod 2**(k-2).  The multiplicative group mod 8, aka
/// the klein 4 group, has 4 elements: 1, 3, 5, and 7.  It is more convenient to use the
/// representation 1, 2**(k-1)-1, 2**(k-1)+1, -1.
///
/// Therefore, we can express each x_i as g**(a_i)*p**(b_i)*q**(c_i) where p and q represent
/// the generators of the klein 4 group, namely 2**(k-1)-1 and 2**(k-1)+1.
/// This turns the difficult to understand recurrence x_i = x_(i-s)*s_(i-r) mod 2**k
/// into three separate linear recurrences which are easy to understand, namely
/// a_i = a_(i-s) + a_(i-r) mod 2**(k-2), the same for b mod 2, and the same for c mod 2.
///
/// h must be odd or the result will be garbage.
/// @param [in] h: power of g to find the exponent of
/// @param [out] g: base of logarithm.  because the multiplicative group of 2**64 is not
/// cyclic, g can be +/-3 or 2**63 +/- 3
/// @return discrete logarithm x so that h = g**x mod 2**64.  The low 62 bits are x.
/// the high 2 bits encode which g to use: 0 -> 3, 1 -> -3, 2 -> 2**63 + 3, 3 -> 2**63 - 3.
/// On failure, 0 is returned, which should only happen if h is even, but still, check if 0
/// is returned and h is not 1.
uint64_t cr8r_prng_log_mod_t64(uint64_t h);

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

#define CR8R_PRNG_LFM_R 127
#define CR8R_PRNG_LFM_S 97

#define CR8R_DEFAULT_PRNG_SM_SEED 0xd20499955ff0e57c

#define CR8R_DEFAULT_PRNG_LCG_SEED 0xe9352d1427990d8e

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

/// Create a PRNG based on a Subtract with Carry Lagged Fibonacci Generator
///
/// WARNING: accesses { @link cr8r_default_prng_splitmix }, all accesses to the default splitmix
/// prng should be done in the same thread or protected by locks.
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
/// (allocation failure; fixup_state should not fail)
cr8r_prng *cr8r_prng_init_lfg_sc(uint64_t seed);

/// Create a PRNG based on a Multiplication Lagged Fibonnacci Generator
///
/// WARNING: accesses { @link cr8r_default_prng_splitmix }, all accesses to the default splitmix
/// prng should be done in the same thread or protected by locks.
/// This should produce higher quality random numbers than the subtract with carry approach
/// (measured by ising model simulation).  It also does not need an extra carry bit
/// in the state.  The considerations when picking such a generator are
/// almost exactly the same as when picking an additive LFG such as
/// subtract with carry, except that all terms in the seed sequence must
/// be odd and the linear recurrence is in the exponents rather than the
/// values directly.
/// @return pointer to new lfg based prng (must be free'd), or NULL on failure
/// (allocation failure; fixup_state should not fail)
cr8r_prng *cr8r_prng_init_lfg_m(uint64_t seed);

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
/// WARNING: accesses { @link cr8r_default_prng_splitmix }, all accesses to the default splitmix
/// prng should be done in the same thread or protected by locks.
/// This PRNG seems to be very fast, closer to an LCG than a bulkier but more typical
/// feedback shift register algorithm like MT, yet still have good properties on most tests.
/// However, it has not been widely used for as long as MT.
/// The seed/state is 32 bytes (4 uint64_t's)
/// Also see { @link cr8r_prng_xoro_jump_t128 } and { @link cr8r_prng_xoro_jump_t192 }
/// @return pointer to new xoroshiro256** based prng (must be free'd), or NULL on failure
/// (allocation; algorithm is seed-agnostic)
cr8r_prng *cr8r_prng_init_xoro(uint64_t seed);

/// Create a PRNG based on Vigna's version of SplitMix
///
/// SplitMix is a light hash applied to a counter.
/// This algorithm passes all testU01 tests, however, it is mostly
/// included to initialize the other PRNGS.
/// @return pointer to new splitmix based prng (must be free'd), or NULL on failure
/// (allocation failure; algorithm is seed-agnostic)
cr8r_prng *cr8r_prng_init_splitmix(uint64_t seed);

/// Default SplitMix prng, automatically used to extend seed values to state values if needed.
extern cr8r_prng *cr8r_default_prng_splitmix;

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

