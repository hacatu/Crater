#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// @section LICENSE
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.
/// @section DESCRIPTION
/// Functions to use a { @link cr8r_vec } as a minmax heap.

#include <stddef.h>
#include <inttypes.h>

#include <crater/vec.h>

/// Turn a vector into a minmax heap in place in linear time
void cr8r_mmheap_ify(cr8r_vec*, cr8r_vec_ft*);

/// Get the min element of a minmax heap
///
/// Simply returns a pointer to the root element, or NULL if len is 0
/// @return pointer to the min element, or NULL if the heap is empty
void *cr8r_mmheap_peek_min(cr8r_vec*, cr8r_vec_ft*);

/// Get the max element of a minmax heap
///
/// Simply returns a pointer to the max element (which is the last element if there are fewer than 3, otherwise
/// the element at index 1 or 2), or NULL if len is 0
/// @return pointer to the min element, or NULL if the heap is empty
void *cr8r_mmheap_peek_max(cr8r_vec*, cr8r_vec_ft*);

/// Move an element up the minmax heap as necessary to restore the heap invariant
///
/// Typically used if one element's value has been changed but the heap invariant is otherwise intact
/// Whether sift_up or sift_down is appropriate for a changed element in a minmax heap depends not only
/// on whether the element was increased or decreased but also on whether it is in a min layer or max layer
/// @param [in] e: pointer to the element to sift up.  Typically you will have this pointer, if you only have the index
/// { @link cr8r_vec_get } can be used to do the self->base + i*ft->base.size calculation for you.
void cr8r_mmheap_sift_up(cr8r_vec*, cr8r_vec_ft*, void *e);

/// Move an element down the minmax heap as necessary to restore the heap invariant
///
/// Typically used if one element's value has been changed but the heap invariant is otherwise intact
/// Whether sift_up or sift_down is appropriate for a changed element in a minmax heap depends not only
/// on whether the element was increased or decreased but also on whether it is in a min layer or max layer
/// @param [in] e: pointer to the element to sift up.  Typically you will have this pointer, if you only have the index
/// { @link cr8r_vec_get } can be used to do the self->base + i*ft->base.size calculation for you.
void cr8r_mmheap_sift_down(cr8r_vec*, cr8r_vec_ft*, void *e);

/// Add a new element to the minmax heap
///
/// @param [in] e: pointer to the element to insert, which is COPIED into the heap
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_mmheap_push(cr8r_vec*, cr8r_vec_ft*, const void *e);

/// Remove the min element from the minmax heap
///
/// @param [in] o: pointer to COPY the heap element to before removing it, cannot be NULL
/// @return 1 on success, 0 on failure (empty heap)
bool cr8r_mmheap_pop_min(cr8r_vec*, cr8r_vec_ft*, void *o);

/// Remove the max element from the minmax heap
///
/// @param [out] o: pointer to COPY the heap element to before removing it, cannot be NULL
/// @return 1 on success, 0 on failure (empty heap)
bool cr8r_mmheap_pop_max(cr8r_vec*, cr8r_vec_ft*, void *o);

/// Remove element at index i from the minmax heap
///
/// Note that if you have a pointer e to an element of a heap vec with ft ft,
/// you can get its index i as i = (e - vec->base)/ft->base.size
/// @param [out] o: pointer to COPY the heap element to before removing it, cannot be NULL
/// @return 1 on success, 0 on failure (empty heap)
bool cr8r_mmheap_pop_idx(cr8r_vec*, cr8r_vec_ft*, void *o, uint64_t i);

/// Push an element and then pop the min element in one operation
///
/// This function allows the common push-pop operation to be optimized,
/// although it is currently no different from just calling both functions in sequence
/// @param [in] e: pointer to the element to insert, which is COPIED into the heap
/// @param [out] o: pointer to COPY the popped element to before removing it, cannot be NULL
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_mmheap_pushpop_min(cr8r_vec*, cr8r_vec_ft*, const void *e, void *o);

/// Push an element and then pop the max element in one operation
///
/// This function allows the common push-pop operation to be optimized,
/// although it is currently no different from just calling both functions in sequence
/// @param [in] e: pointer to the element to insert, which is COPIED into the heap
/// @param [out] o: pointer to COPY the popped element to before removing it, cannot be NULL
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_mmheap_pushpop_max(cr8r_vec*, cr8r_vec_ft*, const void *e, void *o);

