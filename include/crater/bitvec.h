#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// A vector of booleans/bits
///
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <crater/container.h>
#include <crater/prand.h>

/// A bit vector
/// Take care if manipulating these fields directly, this should only
/// be done if the functions in this file are not sufficient
typedef struct{
	/// underlying storage for vector.  Managed by ft->resize.
	uint64_t *buf;
	/// number of bits in the vector.
	uint64_t len;
	/// capacity of buf (in number of bits)
	uint64_t cap;
} cr8r_bvec;

/// Function table for bit vector.
/// Imagine this struct as the class of the bit vector.
/// This struct specifies how to perform necessary operations (allocation, etc).
/// Remember that function tables must be initialized manually.
typedef struct cr8r_bvec_ft cr8r_bvec_ft;
struct cr8r_bvec_ft{
	/// Base function table values (data and size).
	/// Note that the bit vector will always use uint64_t's regardless of what base.size is set to,
	/// so base.size can only potentially affect new_size and resize.  If using general purpose
	/// functions like { @link cr8r_default_resize }, base.size should be set to sizeof(uint64_t).
	cr8r_base_ft base;
	/// determines the capacity to grow to if the bit vector is full but needs to have additional elements added.
	/// Both "cap" and the return value are counted in terms of number of uint64_t's, not number of bytes or bits.
	/// { @link cr8r_default_new_size } is a good choice generally.
	uint64_t (*new_size)(cr8r_base_ft*, uint64_t cap);
	/// resize the backing array to a new size, possibly copying it to a new buffer.
	/// "cap" is counted in terms of number of uint64_t's, not number of bytes or bits.
	/// Must return NULL and "free" buf if the new cap is 0, must "free" NULL (by doing nothing), and must be able to
	/// allocate a new buffer if the current buffer is NULL.  { @link cr8r_default_resize } wraps realloc and free to do this.
	/// If it is not possible to safely move elements, then this function should probably not be allowed to move the allocated buffer
	/// (in the manner realloc does) and should fail instead so that external logic can manage copying to a new vector.
	void *(*resize)(cr8r_base_ft*, void *p, uint64_t cap);
};


/// Initialize a bit vector with an empty buffer of a given capacity
///
/// @param [in] bits: how many bits to reserve space for initially (will be rounded up to a multiple of 64)
/// @return 1 on success, 0 on failure (memory allocation failure)
bool cr8r_bvec_init(cr8r_bvec*, cr8r_bvec_ft*, uint64_t bits);

/// Free the buffer of a bit vector
///
/// ft->resize is called to "resize" to 0.
/// the fields of the vector are all zeroed out
void cr8r_bvec_delete(cr8r_bvec*, cr8r_bvec_ft*);


/// Create a copy of a bit vector
///
/// dest should NOT be initialized or its buffer will be leaked!
/// The copy's capacity is only its length.
/// @param [out] dest: the bit vector to copy TO
/// @param [in] src: the bit vector to copy FROM
/// @return 1 on success, 0 on failure (memory allocation failure)
bool cr8r_bvec_copy(cr8r_bvec *dest, const cr8r_bvec *src, cr8r_bvec_ft*);

/// Create a copy of a slice of a bit vector
///
/// dest should NOT be initialized or its buffer will be leaked!
/// Copies the range [ a : b ) from src to dest.
/// The copy's capacity is only its length (b - a).
/// @param [out] dest: the bit vector to copy TO
/// @param [in] src: the bit vector to copy FROM
/// @param [in] a, b: inclusive start index and exclusive end index
/// @return 1 on success, 0 on failure (memory allocation failure or invalid bounds)
bool cr8r_bvec_sub(cr8r_bvec *dest, const cr8r_bvec *src, cr8r_bvec_ft*, uint64_t a, uint64_t b);


/// Resize a bit vector's reserved buffer.
///
/// Can extend or shrink the buffer, but cannot make it smaller than its length.
/// @param [in] cap: the target capacity, should not be less than the current length
/// @return 1 on success, 0 on failure (memory allocation or invalid bounds)
bool cr8r_bvec_resize(cr8r_bvec*, cr8r_bvec_ft*, uint64_t bits);

/// Resize a bit vector's reserved buffer to its length.
///
/// Exactly like { @link cr8r_bvec_resize } with self->len as the cap parameter
/// @return 1 on success, 0 on failure (memory allocation, shouldn't happen unless ft->resize can fail to shrink)
bool cr8r_bvec_trim(cr8r_bvec*, cr8r_bvec_ft*);


/// Sets len to 0
void cr8r_bvec_clear(cr8r_bvec*);


/// Shuffle the bit vector into a random permutation
///
/// Actually works by counting the number of bits set and
/// redistributing them.
void cr8r_bvec_shuffle(cr8r_bvec*, cr8r_prng*);


/// Get the bit at a given index WITH bounds checking
///
/// @param [in] i: the index to get, should be from 0 inclusive to self->len exclusive
/// @return the bit at index i, or 0 if i is out of bounds
bool cr8r_bvec_get(cr8r_bvec*, uint64_t i);

/// Get the bit at a given index WITHOUT bounds checking
///
/// @param [in] i: the index to get, should be from 0 inclusive to self->len exclusive
/// @return the bit at index i, or 0 if i is out of bounds
bool cr8r_bvec_getu(cr8r_bvec*, uint64_t i);

/// Get the element at a given index, with support for negative indices, WITH bounds checking
///
/// Negative indicies work backwards, with -1 referring to the last element and so on.
/// @param [in] i: the index to get, should be from 0 inclusive to self->len exclusive
/// @return the bit at index i, or 0 if i is out of bounds
bool cr8r_bvec_getx(cr8r_bvec*, int64_t i);

/// Get the element at a given index, with support for negative indices, WITHOUT bounds checking
///
/// Negative indicies work backwards, with -1 referring to the last element and so on.
/// @param [in] i: the index to get, should be from 0 inclusive to self->len exclusive
/// @return the bit at index i, or 0 if i is out of bounds
bool cr8r_bvec_getux(cr8r_bvec*, int64_t i);

/// Set the bit at a given index WITH bounds checking
///
/// @param [in] i: the index to get, should be from 0 inclusive to self->len exclusive
/// @return 1 on success, or 0 if i is out of bounds
bool cr8r_bvec_set(cr8r_bvec*, uint64_t i, bool b);

/// Set the bit at a given index WITHOUT bounds checking
///
/// @param [in] i: the index to get, should be from 0 inclusive to self->len exclusive
void cr8r_bvec_setu(cr8r_bvec*, uint64_t i, bool b);

/// Set the element at a given index, with support for negative indices, WITH bounds checking
///
/// Negative indicies work backwards, with -1 referring to the last element and so on.
/// @param [in] i: the index to get, should be from 0 inclusive to self->len exclusive
/// @return the bit at index i, or 0 if i is out of bounds
bool cr8r_bvec_setx(cr8r_bvec*, int64_t i, bool b);

/// Set the element at a given index, with support for negative indices, WITHOUT bounds checking
///
/// Negative indicies work backwards, with -1 referring to the last element and so on.
/// @param [in] i: the index to get, should be from 0 inclusive to self->len exclusive
void cr8r_bvec_setux(cr8r_bvec*, int64_t i, bool b);

/// Get the length of a bit vector
///
/// Simply returns self->len
/// @return the length of the vector
uint64_t cr8r_bvec_len(cr8r_bvec*);



/// Add a bit to the right hand end of a bit vector
///
/// This is an O(1) operation
/// @param [in] b: the bit to add
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_bvec_pushr(cr8r_bvec*, cr8r_bvec_ft*, bool b);

/// Remove a bit from the right hand end of a bit vector
///
/// Vectors are arranged with increasing indicies at increasing memory addresses, so this is an O(1) operation
/// @param [out] status: if not NULL, *status will be set to 1 on success or 0 on failure (if the bit vector was empty)
/// @return the bit that was removed
bool cr8r_bvec_popr(cr8r_bvec*, int *status);

/// Add a bit to the left hand end of a bit vector.  Avoid if possible.
///
/// Don't do this.  This is an O(n) operation because bit vectors are arranged with increasing indicies at increasing memory addresses.
/// @param [in] b: the bit to add
/// @return 1 on success, 0 on failure
bool cr8r_bvec_pushl(cr8r_bvec*, cr8r_bvec_ft*, bool b);

/// Remove a bit from the left hand end of a bit vector.  Avoid if possible.
///
/// Don't do this.  This is an O(n) operation because bit vectors are arranged with increasing indicies at increasing memory addresses.
/// @param [out] status: if not NULL, *status will be set to 1 on success or 0 on failure (if the bit vector was empty)
/// @return the bit that was removed
bool cr8r_bvec_popl(cr8r_bvec*, int *status);


/// Execute a function on every permutation of a bit vector
///
/// The bit vector is actually permuted in place using Heap's algorithm, and ends at the "last" permutation
/// without being restored.
/// @param [in] f: callback function
/// @param [in, out] data: reentrant data to pass to the callback, can provide input, output, and persistant state
/// without needing to hijack ft->base.data or similar
/// @return 1 on success, 0 on failure (if an array of self->len uint64_t's cannot be allocated)
int cr8r_bvec_forEachPermutation(cr8r_bvec*, void (*f)(const cr8r_bvec*, void *data), void *data);


/// Create a new bit vector by concatenating copies of two given bit vectors
///
/// dest should NOT be initialized or its buffer will be leaked!
/// First, ensures the dest buffer has enough capacity for both src bit vectors, extending if necessary, then copy the bit vectors one after
/// the other.
/// @param [out] dest: bit vector in which to store result
/// @param [in] src_a, src_b: bit vectors to copy elements from
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_bvec_combine(cr8r_bvec *dest, const cr8r_bvec *src_a, const cr8r_bvec *src_b, cr8r_bvec_ft*);

/// Add a copy of another bit vector to a given bit vector
///
/// First, extends self->buf if needed, then copy the other bit vector right after the current end.
/// @param [in, out] self: bit vector to extend with a copy of the other bit vector
/// @param [in] other: bit vector to copy from
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_bvec_augment(cr8r_bvec *self, const cr8r_bvec *other, cr8r_bvec_ft*);


/// Test if all bits are set
bool cr8r_bvec_all(const cr8r_bvec*);

/// Test if any bits
bool cr8r_bvec_any(const cr8r_bvec*);

/// Count the number of bits set
uint64_t cr8r_bvec_popcount(const cr8r_bvec*);

/// Count leading zeros (starting from the largest index self->len - 1)
uint64_t cr8r_bvec_clz(const cr8r_bvec*);

/// Count trailing zeros (starting from the smallest index 0)
uint64_t cr8r_bvec_ctz(const cr8r_bvec*);

/// Count leading ones (starting from the largest index self->len - 1)
uint64_t cr8r_bvec_clo(const cr8r_bvec*);

/// Count trailing ones (starting from the smallest index 0)
uint64_t cr8r_bvec_cto(const cr8r_bvec*);

// TODO: add <<, >>, rotations, reverse

/// Bitwise negate (~) a bit vector in place
void cr8r_bvec_icompl(cr8r_bvec*);

/// Bitwise and (&=) a bit vector with another in place
/// If the other vector is shorter, missing bits are treated as zero
void cr8r_bvec_iand(cr8r_bvec *self, const cr8r_bvec *other);

/// Bitwise or (|=) a bit vector with another in place
/// If the other vector is shorter, missing bits are treated as zero
void cr8r_bvec_ior(cr8r_bvec *self, const cr8r_bvec *other);

/// Bitwise xor (^=) a bit vector with another in place
/// If the other vector is shorter, missing bits are treated as zero
void cr8r_bvec_ixor(cr8r_bvec *self, const cr8r_bvec *other);

/// Test if any bit in the range [a, b) is set
/// If the range is invalid, 0 is returned.
bool cr8r_bvec_any_range(const cr8r_bvec*, uint64_t a, uint64_t b);

/// Test if all bits in the range [a, b) are set
/// If the range is invalid, 0 is returned.
bool cr8r_bvec_all_range(const cr8r_bvec*, uint64_t a, uint64_t b);

/// Set all bits in a range WITH bounds checking
///
/// The bounds are [a, b)
/// @return 1 on success, 0 on failure (out of bounds)
bool cr8r_bvec_set_range(cr8r_bvec*, uint64_t a, uint64_t b, bool v);

/// Lexicographically compare two bit vectors
///
/// @param [in] a, b: vectors to compare
/// @return -1 if a is lexicographically before b, +1 visa versa, or 0 if they are equal
int cr8r_bvec_cmp(const cr8r_bvec *a, const cr8r_bvec *b);

/// Function table for bit vectors
///
/// The default allocation scheme { @link cr8r_default_new_size } and
/// { @link cr8r_default_resize } is used.
extern cr8r_bvec_ft cr8r_bvecft;

