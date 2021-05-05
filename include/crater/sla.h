#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// @section LICENSE
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.
/// @section DESCRIPTION
/// Simple slab allocator.  Allows for efficient allocation of objects of a fixed size

/// Slab allocator
///
/// Aggregates small, fixed size allocations to decrease dynamic memory requests.
/// Maintains an array of "slabs", buffers of many fixed size elements.
/// The unallocated elements are linked together so that unallocated elements can
/// be found and returned and allocated elements can be deallocated trivially.

#include <inttypes.h>
#include <stdbool.h>

typedef struct{
	/// Array of pointers to "slabs" (elements buffers).  Each buffer is twice the size of the last.
	/// New buffers are allocated only whem all elements on all buffers have been allocated.
	/// Creating new buffers in this way allows the backing storage to be increased without needing to move existing allocated elements.
	void **slabs;
	/// Pointer to first unallocated element.  Internally, each unallocated element contains a pointer to the next unallocated element
	/// at the beginning of its memory, hence slab allocators can only be created if the element size is >= sizeof(void*)
	void *first_elem;
	/// Number of slabs
	uint64_t slabs_len;
	/// Capacity of last slab
	uint64_t slab_cap;
	/// Size of a single element
	uint64_t elem_size;
} cr8r_sla;

/// Initialize a slab allocator
///
/// @param [out] self: slab allocator to initialize
/// @param [in] elem_size: size of a single element in bytes.  allocations returned will be this big.  must be at least sizeof(void*)
/// @param [in] cap: the number of elements to reserve space for initially, which is all reserved in a single "slab".  must be at least 1
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_sla_init(cr8r_sla *self, uint64_t elem_size, uint64_t cap);

/// Delete a slab allocator
///
/// Frees all slabs and zeros out capacity/len.  The allocator should not be used again after this, unless it is reinitialized.
/// @param [in, out] self: slab allocator to delete
void cr8r_sla_delete(cr8r_sla *self);

/// Allocate an object
///
/// If successful, the returned pointer will point to uninitialized memory with the element size of the slab allocator.
/// @param [in] self: slab allocator to allocate from
/// @return pointer to useable uninitialized memory of the slab allocator's element size,
/// or NULL if no unallocated elements are available and allocation of more backing storage fails.
void *cr8r_sla_alloc(cr8r_sla *self);

/// Frees and object which was previously allocated by { @link cr8r_sla_alloc }
///
/// The slab allocator does not check if this is indeed an allocated block, because doing so would make
/// deallocation an O(log(n)) operation and require a larger minimum element size be enforced.
/// It is technically possible to "abuse" this function to "free" pointers to elements from some other backing store,
/// and then the allocator will hand these out together with elements actually from the underlying slabs.
/// However, this should typically not be done because it ties the slab allocator to an external backing store
/// that is not managed by it.
/// @param [in] self: slab allocator to work with
/// @param [in] p: pointer to free
void cr8r_sla_free(cr8r_sla *self, void *p);

