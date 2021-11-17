#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// Functions to use a { @link cr8r_vec } as a heap.
///
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <stddef.h>
#include <inttypes.h>

#include <crater/vec.h>

/// Turn a vector into a heap in place in linear time
///
/// @param [in] ord: 1 to use a max heap (use ft->cmp directly), -1 to use a min heap (invert ft->cmp)
void cr8r_heap_ify(cr8r_vec*, cr8r_vec_ft*, int ord);

/// Get the top (typically max) element of a heap
///
/// Simply returns the vector's buffer, or NULL if len is 0
/// @return pointer to the top (typically max) element, or NULL if the heap is empty
void *cr8r_heap_top(cr8r_vec*, cr8r_vec_ft*);

/// Move an element up the heap as necessary to restore the heap invariant
///
/// Typically used if one element's value has been increased but the heap invariant is otherwise intact
/// @param [in] e: pointer to the element to sift up.  Typically you will have this pointer, if you only have the index
/// { @link cr8r_vec_get } can be used to do the self->base + i*ft->base.size calculation for you.
/// @param [in] ord: 1 to use a max heap (use ft->cmp directly), -1 to use a min heap (invert ft->cmp)
void cr8r_heap_sift_up(cr8r_vec*, cr8r_vec_ft*, void *e, int ord);

/// Move an element down the heap as necessary to restore the heap invariant
///
/// Typically used if one element's value has been decreased but the heap invariant is otherwise intact
/// @param [in] e: pointer to the element to sift up.  Typically you will have this pointer, if you only have the index
/// { @link cr8r_vec_get } can be used to do the self->base + i*ft->base.size calculation for you.
/// @param [in] ord: 1 to use a max heap (use ft->cmp directly), -1 to use a min heap (invert ft->cmp)
void cr8r_heap_sift_down(cr8r_vec*, cr8r_vec_ft*, void *e, int ord);

/// Add a new element to the heap
///
/// @param [in] e: pointer to the element to insert, which is COPIED into the heap
/// @param [in] ord: 1 to use a max heap (use ft->cmp directly), -1 to use a min heap (invert ft->cmp)
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_heap_push(cr8r_vec*, cr8r_vec_ft*, const void *e, int ord);

/// Remove the top (typically max) element from the heap (has no relevant ordering guarantee)
///
/// @param [in] o: pointer to COPY the heap element to before removing it, cannot be NULL
/// @param [in] ord: 1 to use a max heap (use ft->cmp directly), -1 to use a min heap (invert ft->cmp)
/// @return 1 on success, 0 on failure (empty heap)
bool cr8r_heap_pop(cr8r_vec*, cr8r_vec_ft*, void *o, int ord);

