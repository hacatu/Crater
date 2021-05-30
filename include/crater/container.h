#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// @section LICENSE
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.
/// @section DESCRIPTION
/// Base function table for Crater containers

#include <inttypes.h>

/// Base function table for all Crater containers
///
/// This structure is always present at the beginning of the function table for all Crater containers,
/// and callbacks in function tables take a pointer to this structure as their first argument.
typedef struct{
	/// Custom "class" data which is passed to the callbacks in this table.  This is intended so that a single callback can work in multiple ways,
	/// instead of writing many similar functions.  For example, if the elements are triples of uint64_t's (with some additional value data), this
	/// data field could simply be a number 0, 1, or 2, specifying which coordinate to compare by, rather than writing three separate comparison functions.
	void *data;
	/// how large each element is, in bytes
	uint64_t size;
} cr8r_base_ft;


/// "Default" ft->new_size implementation (for vectors)
///
/// Doubles the capacity if it is nonzero, otherwise returns 8.
/// @param [in] cap: the capacity before resizing
/// @return what capacity the vector should grow to
uint64_t cr8r_default_new_size(cr8r_base_ft*, uint64_t cap);

/// "Default" ft->resize implementation (for vectors)
///
/// Calls "free" if the given capacity is 0 (note that freeing NULL is a valid no-op),
/// otherwise calls realloc (note that reallocing NULL is valid and allocates a new buffer).
/// @param [in] p: the buffer to resize
/// @param [in] cap: the capacity to resize to
/// @return a pointer to the resized buffer, or NULL on failure or if cap is 0
void *cr8r_default_resize(cr8r_base_ft*, void *p, uint64_t cap);

/// "Default" do-nothing ft->resize implementation (for vectors)
///
/// Always returns NULL, indicating the buffer could not be resized.
/// This is convenient if you want to wrap a stack/static/thread local
/// array as a vector, since such an array cannot be resized.
/// @return NULL
void *cr8r_default_resize_pass(cr8r_base_ft*, void *p, uint64_t cap);

/// "Default" ft->cmp implementation
///
/// Calls memcmp(a, b, ft->base.size)
/// WARNING: this may have problems with padded structs or doubles
/// since equal values can have different representations in memory.
/// A custom implementation is preferable for padded structs,
/// anything containing doubles, or scalar types.
/// @param [in] a, b: the elements to compare
/// @return -1 if a < b, 0 if a == b, 1 if a > b
int cr8r_default_cmp(const cr8r_base_ft*, const void *a, const void *b);

/// "Default" ft->swap implementation (for vectors)
///
/// Creates a temporary buffer and swaps with memcpy, if a != b.
/// @param [in] a, b: the elements to swap
void cr8r_default_swap(cr8r_base_ft*, void *a, void *b);

/// "Default" ft->hash implementation for uint64_t (for hash tables)
///
/// Multiplies the value by a fixed prime and xors the high and low words of the 128 bit result
uint64_t cr8r_default_hash_u64(const cr8r_base_ft*, const void*);

/// "Default" ft->hash implementation (for hash tables)
///
/// WARNING: this may have problems with padded structs or doubles
/// since equal values can have different representations in memory.
/// A custom implementation is preferable for padded structs,
/// anything containing doubles, or scalar types.
uint64_t cr8r_default_hash(const cr8r_base_ft*, const void*);

/// "Default" ft->hash implementation for null terminated strings (for hash tables)
///
/// Uses the djb2 algorithm
uint64_t cr8r_default_hash_cstr(const cr8r_base_ft*, const void*);

/// "Default" ft->cmp implementation for uint64_t
int cr8r_default_cmp_u64(const cr8r_base_ft*, const void*, const void*);

/// "Default" ft->cmp implementation for null terminated strings
int cr8r_default_cmp_cstr(const cr8r_base_ft*, const void*, const void*);

/// "Default" ft->add implementation (for avl trees and hash tables)
///
/// Simply replaces the existing value with the new value
int cr8r_default_replace(cr8r_base_ft*, void *_a, void *_b);

/// ft->free implementation (for avl trees or circular lists) that does nothing
///
/// Useful to allow temporarily adding stack allocated nodes,
/// or to temporarily disable freeing nodes so one can be removed but still have some
/// processing done on it via an existing pointer before freeing it.
void cr8r_default_free_pass(cr8r_base_ft*, void*);

/// ft->alloc implementation (for avl trees or circular lists)
///
/// Wraps a slab allocator.  WARNING: ft->base.data must point to a slab
/// allocator ( { @link cr8r_sla } ) with the appropriate element size.
/// Remember, the element size of the slab allocator should be the node size,
/// not the element size, of the container (avl tree or circular list).
/// See { @link cr8r_avl_ft_initsla } and { @link cr8r_cll_ft_initsla }.
void *cr8r_default_alloc_sla(cr8r_base_ft*);

/// ft->free implementation (for avl trees or circular lists)
///
/// Wraps a slab allocator.  WARNING: ft->base.data must point to a slab
/// allocator ( { @link cr8r_sla } ) with the appropriate element size.
/// Remember, the element size of the slab allocator should be the node size,
/// not the element size, of the container (avl tree or circular list).
/// See { @link cr8r_cll_ft_initsla } and { @link cr8r_cll_ft_initsla }.
void cr8r_default_free_sla(cr8r_base_ft*, void*);

/// ft->del implementation (for hash tables or vectors)
///
/// Wraps free().  Keep in mind that the second argument is
/// a pointer to an element and is dereferenced before being freed.
/// For example, malloc'd C strings are stored as single pointers,
/// so a vector of C strings is really a vector of pointers, and
/// a pointer to an element of such a vector is a pointer to a
/// pointer to some null terminated character sequence.
void cr8r_default_free(cr8r_base_ft*, void*);

/// Raise b to the power of e modulo n using binary exponentiation
uint64_t cr8r_powmod(uint64_t b, uint64_t e, uint64_t n);

/// Cast a flexible length array member to a given type using union hackery sorcery
#define CR8R_FLA_CAST(T, p) (((union{__typeof__(p) data; T a;})(p)).a)

