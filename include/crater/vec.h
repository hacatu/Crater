#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// @section LICENSE
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.
/// @section DESCRIPTION
/// Simple, featureful generic vector

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <crater/container.h>
#include <crater/prand.h>

/// A vector
/// Take care if manipulating these fields directly, this should only
/// be done if the functions in this file are not sufficient
typedef struct{
	/// underlying storage for vector.  Managed by ft->resize.
	void *buf;
	/// number of elements in the vector.
	uint64_t len;
	/// capacity of buf (in number of elements)
	uint64_t cap;
} cr8r_vec;

/// Function table for vector.
/// Imagine this struct as the class of the vector.
/// This struct specifies the size of the elements in a vector in bytes,
/// as well as how to perform necessary operations (compare, copy, etc).
/// Remember that function tables must be initialized manually.  In particular,
/// the cr8r_vec_default_* functions can be used as decent defaults, but must be explicitly set.
typedef struct cr8r_vec_ft cr8r_vec_ft;
struct cr8r_vec_ft{
	cr8r_base_ft base;
	/// determines the capacity to grow to if the vector is full but needs to have additional elements added
	/// { @link cr8r_vec_default_new_size } is a good choice generally.
	uint64_t (*new_size)(cr8r_base_ft*, uint64_t cap);
	/// resize the backing array to a new size, possibly copying it to a new buffer.
	/// Must return NULL and "free" buf if the new cap is 0, must "free" NULL (by doing nothing), and must be able to
	/// allocate a new buffer if the current buffer is NULL.  { @link cr8r_vec_default_resize } wraps realloc and free to do this.
	/// If it is not possible to safely move elements, then this function should probably not be allowed to move the allocated buffer
	/// (in the manner realloc does) and should fail instead so that external logic can manage copying to a new vector.
	void *(*resize)(cr8r_base_ft*, void *p, uint64_t cap);
	/// delete an element of the vector.  This only needs to clean up data owned by the element,
	/// not the element itself because it lives in the buffer.  Can also be used to scramble memory if desired.
	/// Useful for strings, file descriptors, etc.  Can be NULL if no operation is needed.
	void (*del)(cr8r_base_ft*, void *p);
	/// copy an element of the vector.  Can be NULL if memcpy is sufficient, otherwise this function must perform the role of memcpy
	/// plus whatever else it needs to do.
	void (*copy)(cr8r_base_ft*, void *dest, const void *src);
	/// compare two elements of the vector.  { @link cr8r_vec_default_cmp } is a suitable default if needed.
	/// should return <0 if a < b, 0 if a == b, and >0 if a > b.  should not return INT_MIN.
	int (*cmp)(const cr8r_base_ft*, const void *a, const void *b);
	/// swap two elements.  The default { @link cr8r_vec_default_swap } simply swaps the two elements using memcpy and a temporary buffer.
	/// more optimized versions can be used if relevant.  Should work even if a == b, when it should do nothing.
	void (*swap)(cr8r_base_ft*, void *a, void *b);
};


/// Convenience function to initialize a { @link cr8r_vec_ft }
///
/// Using standard structure initializer syntax with designated initializers may be simpler.
/// However, this function provides basic checking (it checks the required functions aren't NULL).
/// @param [in] data: pointer to user defined data to associate with the function table.
/// generally NULL is sufficient.  see { @link cr8r_base_ft } for a more in-depth explaination
/// @param [in] size: size of a single element in bytes
/// @param [in] new_size: called to determine the capacity to grow to given the previous capacity (both in number of elements)
/// @param [in] resize: memory management function.  Should "free" the buffer if cap is 0 (and do nothing if the current buffer is NULL).
/// Should allocate a new buffer if the current buffer is NULL and cap is not 0.  Otherwise, should resize the buffer to the requested size,
/// possibly copying it to a new address.  See { @link cr8r_default_resize } for a basic generic implementation.
/// @param [in] del: called on any element before deleting it.  can be NULL if no action is required.
/// @param [in] copy: copy an element.  can be NULL if memcpy is sufficient.
/// @param [in] cmp: comparison function.  must not be NULL to call search and sort functions.
/// @param [in] swap: swap two element.  only required for a few functions (namely { @link cr8r_vec_shuffle }).  { @link cr8r_default_swap }
/// is a good generic choice.
/// @return 1 on success, 0 on failure (if resize is NULL)
bool cr8r_vec_ft_init(cr8r_vec_ft*,
	void *data, uint64_t size,
	uint64_t (*new_size)(cr8r_base_ft*, uint64_t cap),
	void *(*resize)(cr8r_base_ft*, void *p, uint64_t cap),
	void (*del)(cr8r_base_ft*, void *p),
	void (*copy)(cr8r_base_ft*, void *dest, const void *src),
	int (*cmp)(const cr8r_base_ft*, const void *a, const void *b),
	void (*swap)(cr8r_base_ft*, void *a, void *b)
);


/// Initialize a vector with an empty buffer of a given capacity
///
/// @param [in] cap: how many entries to reserve space for initially
/// @return 1 on success, 0 on failure (memory allocation failure)
bool cr8r_vec_init(cr8r_vec*, cr8r_vec_ft*, uint64_t cap);

/// Delete all entries in a vector and free its buffer
///
/// ft->del (can be NULL) is called on each element, then ft->resize is called to "resize" to 0.
/// the fields of the vector are all zeroed out
void cr8r_vec_delete(cr8r_vec*, cr8r_vec_ft*);


/// Create a copy of a vector
///
/// Copies entries with ft->copy if applicable.
/// The copy's capacity is only its length.
/// @param [out] dest: the vector to copy TO
/// @param [in] src: the vector to copy FROM
/// @return 1 on success, 0 on failure (memory allocation failure)
bool cr8r_vec_copy(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft*);

/// Create a copy of a slice of a vector
///
/// Copies the range [ a : b ) from src to dest.
/// Copies entries with ft->copy if applicable.
/// The copy's capacity is only its length (b - a).
/// @param [out] dest: the vector to copy TO
/// @param [in] src: the vector to copy FROM
/// @param [in] a, b: inclusive start index and exclusive end index
/// @return 1 on success, 0 on failure (memory allocation failure or invalid bounds)
bool cr8r_vec_sub(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft*, uint64_t a, uint64_t b);


/// Resize a vector's reserved buffer.
///
/// Can extend or shrink the buffer, but cannot make it smaller than its length.
/// @param [in] cap: the target capacity, should not be less than the current length
/// @return 1 on success, 0 on failure (memory allocation or invalid bounds)
bool cr8r_vec_resize(cr8r_vec*, cr8r_vec_ft*, uint64_t cap);

/// Resize a vector's reserved buffer to its length.
///
/// Exactly like { @link cr8r_vec_resize } with self->len as the cap parameter
/// @return 1 on success, 0 on failure (memory allocation, shouldn't happen unless ft->resize can fail to shrink)
bool cr8r_vec_trim(cr8r_vec*, cr8r_vec_ft*);


/// Remove all elements from a vector
///
/// Calls ft->del if applicable, and sets len to 0
void cr8r_vec_clear(cr8r_vec*, cr8r_vec_ft*);


/// Shuffle the vector into a random permutation
///
/// The algorithm produces permutations uniformly in theory, although in practice
/// the number of permutations of a sufficiently large vector is enormously larger
/// than the state space of any pseudorandom number generator.  This won't
/// cause any remotely meaningful patterns.
void cr8r_vec_shuffle(cr8r_vec*, cr8r_vec_ft*, cr8r_prng*);


/// Get the element at a given index
///
/// Similar to just doing self->buf + i*ft->base.size (recall that void pointer arithmetic acts like char pointer arithmetic),
/// except that this function does bounds checking.
/// @param [in] i: the index to get, should be from 0 inclusive to self->len exclusive
/// @return a pointer to the element at index i, or NULL if i is out of bounds
void *cr8r_vec_get(cr8r_vec*, const cr8r_vec_ft*, uint64_t i);

/// Get the element at a given index, with support for negative indices
///
/// Negative indicies work backwards, with -1 referring to the last element and so on.
/// @param [in] i: the index to get, should be from 0 inclusive to self->len exclusive
/// @return a pointer to the element at index i, or NULL if i is out of bounds
void *cr8r_vec_getx(cr8r_vec*, const cr8r_vec_ft*, int64_t i);

/// Get the length of a vector
///
/// Simply returns self->len
/// @return the length of the vector
uint64_t cr8r_vec_len(cr8r_vec*);


/// Add an element to the right hand end of a vector
///
/// Vectors are arranged with increasing indicies at increasing memory addresses, so this is an O(1) operation
/// @param [in] e: the element to add, which is copied from this pointer into the vector
/// @return 1 on success, 0 on failure
bool cr8r_vec_pushr(cr8r_vec*, cr8r_vec_ft*, const void *e);

/// Remove an element from the right hand end of a vector
///
/// Vectors are arranged with increasing indicies at increasing memory addresses, so this is an O(1) operation
/// @param [out] o: the removed element is copied here from the vector
/// @return 1 on success, 0 on failure (ie if the vector was empty)
bool cr8r_vec_popr(cr8r_vec*, cr8r_vec_ft*, void *o);

/// Add an element to the left hand end of a vector
///
/// This is an O(n) operation because vectors are arranged with increasing indicies at increasing memory addresses.
/// @param [in] e: the element to add, which is copied from this pointer into the vector
/// @return 1 on success, 0 on failure
bool cr8r_vec_pushl(cr8r_vec*, cr8r_vec_ft*, const void *e);

/// Remove an element from the left hand end of a vector
///
/// This is an O(n) operation because vectors are arranged with increasing indicies at increasing memory addresses.
/// @param [out] o: the removed element is copied here from the vector
/// @return 1 on success, 0 on failure (ie if the vector was empty)
bool cr8r_vec_popl(cr8r_vec*, cr8r_vec_ft*, void *o);


/// Filter a vector in-place
///
/// ft->del is called on elements that fail the predicate, if applicable
/// @param [in] pred: predicate to check elements with
/// @return 1 on success, 0 on failure (memory allocation, shouldn't happen unless ft->resize can fail to shrink)
bool cr8r_vec_filter(cr8r_vec*, cr8r_vec_ft*, bool (*pred)(const void*));

/// Create a new vector as a subsequence matching a predicate
///
/// Iterate over the elements of a vector and copy them into a new vector if they match the predicate.
/// ft->copy function is called if applicable.
/// @param [out] dest: vector to fill with filtered subsequence, should have a cap of 0 or valid buffer
/// @param [in] src: vector to copy filtered sequence from
/// @param [in] pred: predicate to check elements with
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_vec_filtered(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft*, bool (*pred)(const void*));

/// Create a new vector by applying a transformation function to each element
///
/// This creates a new vector, whose element type and entire function table may be different.
/// The transformation funtion also must perform any relevant work that ft->copy would have to,
/// since the output element type is not necessarily the same as the input element type.
/// @param [out] dest: vector to fill with outputs of transformation function, should have a cap of 0 or valid buffer
/// @param [in] src: vector of elements to pass to the transformation function
/// @param [in] src_ft: function table for src vector { @link cr8r_vec_ft }
/// @param [in] dest_ft: function table for dest vector { @link cr8r_vec_ft }
/// @param [in] f: transformation function.  The first argument, o, is a pointer to the element in the destination vector to output to,
/// and the second argument, e, is a pointer to the element in the source vector to read from
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_vec_map(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft *src_ft, cr8r_vec_ft *dest_ft, void (*f)(void *o, const void *e));

/// Execute a function on every permutation of a vector
///
/// The vector is actually permuted in place using Heap's algorithm, and ends at the "last" permutation
/// without being restored.
/// @param [in] f: callback function.  given the entire permuted vector and function table.
/// @return 1 on success, 0 on failure (if an array of self->len uint64_t's cannot be allocated)
int cr8r_vec_forEachPermutation(cr8r_vec*, cr8r_vec_ft*, void (*f)(const cr8r_vec*, const cr8r_vec_ft*));


/// Create a new vector by concatenating copies of two given vectors
///
/// First, ensures the dest buffer has enough capacity for both src vectors, extending if necessary, then copy the vectors one after
/// the other.  ft->copy is called if applicable
/// @param [out] dest: vector in which to store result, should have a cap of 0 or valid buffer
/// @param [in] src_a, src_b: vectors to copy elements from
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_vec_combine(cr8r_vec *dest, const cr8r_vec *src_a, const cr8r_vec *src_b, cr8r_vec_ft*);

/// Add a copy of another vector to a given vector
///
/// First, extends self->buf if needed, then copy the other vector right after the current end.
/// ft->copy is called if applicable
/// @param [in, out] self: vector to extend with a copy of the other vector
/// @param [in] other: vector to copy from
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_vec_augment(cr8r_vec *self, const cr8r_vec *other, cr8r_vec_ft*);


/// Test if a predicate holds for all elements in a vector
///
/// Returns false immediately if any element does not satisfy the predicate.
/// @param [in] pred: predicate function, called on each element in the vector
/// @return 1 if the predicate function returns 1 on all elements, 0 (as soon as it doesn't) otherwise
bool cr8r_vec_all(const cr8r_vec*, const cr8r_vec_ft*, bool (*pred)(const void*));

/// Test if a predicate holds for any element in a vector
///
/// Returns true immediately if any element satisfies the predicate.
/// Note that this is completely equivalent to the negation of { @link cr8r_vec_all }
/// with the negation of the predicate.
/// @param [in] pred: predicate function, called on each element in the vector
/// @return 1 (as soon as) the predicate function returns 1 on any element, 0 if it returns 0 on all of them
bool cr8r_vec_any(const cr8r_vec*, const cr8r_vec_ft*, bool (*pred)(const void*));

/// Check if a vector contains a given element
///
/// This is O(n) because it does not assume a sorted vector and simply does a linear search.
/// See { @link HC_VEC_containss } for a binary search version that does require a sorted vector.
/// @param [in] e: element to search for (using ft->cmp)
/// @return 1 if the vector contains the element, 0 otherwise
bool cr8r_vec_contains(const cr8r_vec*, const cr8r_vec_ft*, const void *e);

/// Get the first index of an element in a vector
///
/// This is O(n) because it does not assume a sorted vector and simply does a linear search.
/// See { @link HC_VEC_indexs } for a binary search version that does require a sorted vector.
/// Returns -1 on failure.
/// @param [in] e: element to search for (using ft->cmp)
/// @return the index of the element if it is in the vector, or -1 otherwise
int64_t cr8r_vec_index(const cr8r_vec*, const cr8r_vec_ft*, const void *e);


/// Perform a right fold on a vector
///
/// Given an initial accumulator value of some type T in a void pointer, a vector, and a function that takes a void pointer to
/// a T value and an element of the vector and returns a void pointer to a T value, this function repeatedly applies
/// that function to the current accumulator value and vector element to get the new accumulator value.
/// This is repeated for every element in the vector, going from left to right (low indices to high),
/// and the final accumulator value is returned.
/// The accumulation function must handle freeing old accumulator values once it is finished with them if necessary.
/// @param [in] init: starting value for the accumulator
/// @param [in] f: accumulation function: should take current accumulator value (as void pointer) and current vector element
/// and return new accumulator value.  It should either modify the accumulator value in place, returning it as well,
/// or allocate a new pointer for the new accumulator value and free the old one once the new one has been computed.  In cases
/// where the accumulator value is an integer or something else that fits within a pointer, the pointer can be used as a casted
/// integer or whatnot instead of as a pointer.
/// @return the final accumulator value
void *cr8r_vec_foldr(const cr8r_vec*, const cr8r_vec_ft*, void *init, void *(*f)(void *acc, const void *e));

/// Perform a left fold on a vector
///
/// Exactly the same as { @link cr8r_vec_foldr } except vector entries are processed from right to left (high indices to low) instead
/// of left to right (low to high).
/// @param [in] init: starting value for the accumulator
/// @param [in] f: accumulation function
/// @return the final accumulator value
void *cr8r_vec_foldl(const cr8r_vec*, const cr8r_vec_ft*, void *init, void *(*f)(void *acc, const void *e));


/// Sort a vector in-place according to ft->cmp
///
/// Currently uses heapsort, so average and worst case time complexities are both O(n*log(n)).
void cr8r_vec_sort(cr8r_vec*, cr8r_vec_ft*);

/// Create a sorted copy of a vector
///
/// Simply calls { @link cr8r_vec_copy } followed by { @link cr8r_vec_sort } on the copy
/// @param [out] dest: the vector to copy to and sort
/// @param [in] src: the vector to copy from
/// @return 1 on success, 0 on failure (memory allocation failure)
bool cr8r_vec_sorted(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft*);

/// Reverse a vector in place
void cr8r_vec_reverse(cr8r_vec*, cr8r_vec_ft*);

/// Create a reversed copy of a vector
///
/// copies elements in reverse order, using ft->copy if applicable
/// @param [out] dest: the vector to store the reversed copy in
/// @param [in] src: the vector to create a reversed copy from
/// @return 1 on success, 0 on failure (memory allocation failure)
bool cr8r_vec_reversed(cr8r_vec *dest, const cr8r_vec *src, cr8r_vec_ft*);

/// Check if a sorted vector contains an element
///
/// The vector should be in ascending order according to ft->cmp.
/// Uses binary search, so the average and worst case time complexities are O(log(n)).
/// If the elements are sorted and belong to some known distribution, some form of interpolation
/// seach could work even faster, but this is best implemented as a custom function.
/// @param [in] e: element to search for (using ft->cmp)
/// @return 1 if the vector contains the element, 0 otherwise
bool cr8r_vec_containss(const cr8r_vec*, const cr8r_vec_ft*, const void *e);

/// Get the index of an element in a sorted vector
///
/// The vector should be in ascending order according to ft->cmp.
/// If there are multiple elements which compare equal to the query, the index of any one of them may be returned.
/// Uses binary search, so the average and worst case time complexities are O(log(n)).
/// If the elements are sorted and belong to some known distribution, some form of interpolation
/// seach could work even faster, but this is best implemented as a custom function.
/// @param [in] e: element to search for (using ft->cmp)
/// @return index of the element in the vector, or -1 if not present
int64_t cr8r_vec_indexs(const cr8r_vec*, const cr8r_vec_ft*, const void *e);

/// Lexicographically compare two vectors
///
/// @param [in] a, b: vectors to compare
/// @return -1 if a is lexicographically before b, +1 visa versa, or 0 if they are equal
int cr8r_vec_cmp(const cr8r_vec *a, const cr8r_vec *b, const cr8r_vec_ft*);

/// Pick a pivot for { @link cr8r_vec_partition } using the median of 3 approach
///
/// This is an O(1) pivot selection algorithm that can cause quicksort/quickselect
/// to perform badly (O(n**2)) in pathological cases, but most of the time works
/// very well (O(n*log(n)) and faster than { @link cr8r_vec_pivot_mm } in practice
/// most of the time).
/// Picks the median of the first, middle (rounded down), and last elements.
/// In the event of ties, which valid element is chosen is not specified,
/// but is consistent for the same elements.
/// @param [in] self: vector to pick a pivot for (a subrange of), is not modified
/// @param [in] a, b: inclusive, exclusive indices of the subrange to pick a pivot for
/// @return a pointer to the selected pivot, or NULL if a and b are not valid subrange
/// indices for the given vector.
void *cr8r_vec_pivot_m3(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b);

/// Pick a pivot for { @link cr8r_vec_partition } using the median of medians approach
///
/// This is an O(n) pivot selection algorithm that ensures quicksort/quickselect
/// will always run in O(n*log(n)) time.  However, because the entire subrange is
/// processed, this will often end up slower than { @link cr8r_vec_pivot_m3 }.
/// The reason median of medians ensures quicksort/quickselect will finish in
/// O(n*log(n)) time is because it will select a pivot whose index is within a certain
/// ratio of the median (eg if the subrange has k elements then the pivot selected will
/// be one of the central a*k elements for some small fixed coefficient).
/// The median of medians algorithm used does not allocate extra space, but
/// the order of the given subrange is not preserved.
/// Picks the median of the first, middle (rounded down), and last elements.
/// In the event of ties, which valid element is chosen is not specified,
/// but is consistent for the same elements.
/// @param [in, out] self: vector to pick a pivot for (a subrange of), will be
/// reordered in place, but only within the given subrange
/// @param [in] a, b: inclusive, exclusive indices of the subrange to pick a pivot for
/// @return a pointer to the selected pivot, or NULL if a and b are not valid subrange
/// indices for the given vector.  because the target subrange is reordered so the algorithm
/// can run in place, the pivot will always end up at index a, but this should not be
/// relied on.
void *cr8r_vec_pivot_mm(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b);

/// Partition a subrange of a vector into elements < a pivot and elements >= a pivot
///
/// The given subrange is rearranged so that all elements before the pivot
/// are < the pivot and all elements >= the pivot are after the pivot, and a pointer
/// to the pivot is returned
/// @param [in,out] self: vector to partition a subrange of in place
/// @param [in] a, b: inclusive, exclusive bounds of subrange (ie, consider elements [a, b))
/// @param [in] piv: a pointer to the pivot
/// @return pointer to the pivot, which was placed after all elements < itself but before
/// all other elements >= itself
void *cr8r_vec_partition(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b, void *piv);

/// Find the ith element of a subrange of a vector without completely sorting it
///
/// The given subrange is partitioned in place, but the quickselect algorithm
/// is used rather than sorting so the expected running time is linear.  Using
/// median of medians would guarantee linear time, but median of 3 is good enough
/// most of the time.
/// @param [in,out] self: vector to find ith element of.  May be reoredered.
/// @param [in] a, b: inclusive, exclusive bounds of subrange
/// @param [in] i: index to find.  For example, 0 finds the smallest element, 1 the
/// second smallest, and so on.
void *cr8r_vec_ith(cr8r_vec *self, cr8r_vec_ft *ft, uint64_t a, uint64_t b, uint64_t i);

/// Callback for { @link cr8r_vec_foldr } to sum up elements in vector
///
/// The first argument is treated as a uint64_t, NOT a pointer to uint64_t.
/// Thus the call can be (uint64_t)cr8r_vec_foldr(self, ft, (void*)0, cr8r_default_acc_sum_u64).
void *cr8r_default_acc_sum_u64(void*, const void*);

/// Callback for { @link cr8r_vec_foldr } to sum up powers of elements in a vector mod a number
///
/// The first argument MUST be a pointer to three consecutive uint64_t's: the accumulator,
/// the exponent, and the modulus, respectively.  The same pointer is returned: the accumulator
/// is modified in place.
void *cr8r_default_acc_sumpowmod_u64(void*, const void*);

/// Function table for vectors of uint64_t's
///
/// Trivial copy/swap and { @link cr8r_default_cmp_u64 } are used,
/// plus the default allocation scheme { @link cr8r_default_new_size } and
/// { @link cr8r_default_resize }.
extern cr8r_vec_ft cr8r_vecft_u64;

/// Threshold below which quicksort and quickselect will switch to using insertion sort
#define CR8R_VEC_ISORT_BOUND 16

