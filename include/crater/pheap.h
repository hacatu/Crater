#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// Intrusive pairing heap.  This data structure is INTRUSIVE, whereas
/// most data structures in Crater are polymorphic.  These are both
/// ways of implementing generic data structures.  Polymorphic
/// in this case simply means that the data structure is implemented as
/// as structs which contain metadata and element data.  For example,
/// avl tree nodes contain child pointers, balance factor (metadata),
/// and the `char data[]` field (element data, stored polymorphically
/// as a flexible length array).
///
/// Intrusive data structures on the other hand have all the metadata
/// stored in a struct which is the same for any type of element data,
/// and then specialization to different types is handled by creating
/// structs containing this metadata struct.  EG, for a pairing heap,
/// any struct containing a metadata struct of the correct type can
/// be used as a pairing heap.
///
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <crater/container.h>
#include <crater/prand.h>

typedef struct cr8r_pheap_node cr8r_pheap_node;
struct cr8r_pheap_node{
	/// pointer to the first child (or NULL).  All children form a circularly linked list
	cr8r_pheap_node *first_child;
	/// pointer to the next sibling.  These are the pointers that form a cll.
	cr8r_pheap_node *sibling;
	/// pointer to the parent
	cr8r_pheap_node *parent;
};

/// function table for pairing heaps
/// note that since this is an intrusive data structure,
/// base.size is actually the OFFSET of the cr8r_pheap_node
/// struct within the outer struct, and cmp, alloc, and free
/// deal with outer struct pointers.
typedef struct{
	/// Base function table values (data and size)
	cr8r_base_ft base;
	/// Function to compare elements.  Should only depend on "key" data within the elements.
	/// Should return 0 for elements that are equal, or any nonzero int for elements that are not equal.
	/// Equality according to this function should imply equal hash values.
	/// Currently no requirement is placed on return values for unequal operands beyond being nonzero,
	/// but if possible <0 should indicate the first operand is smaller, >0 should indicate the first argument is large,
	/// and the function should avoid returning INT_MIN
	///
	/// some cmps are unsequenced so data really must not be modified here
	int (*cmp)(const cr8r_base_ft*, const void*, const void*);
	/// Function to allocate a new node.
	/// This should allocate base.size + sizeof(cr8r_pheap_node) bytes
	/// (remember for this data structure base.size is actaully the offset of the cr8r_pheap_node within the outer struct).
	/// The slab allocator in this library is designed
	/// to work well for allocating nodes
	void *(*alloc)(cr8r_base_ft*);
	/// Function to deallocate a node.
	void (*free)(cr8r_base_ft*, void*);
} cr8r_pheap_ft;

/// Allocate a new pairing heap node using ft->alloc and initialize it.
/// The pointer returned is a pointer to the outer struct which contains
/// the cr8r_pheap_node struct.
///
/// Note that this function is not really needed in the normal use case
/// for this data structure.  Typically, the pairing heap will be a
/// secondary data structure stored in place within a hash table, avl tree,
/// or so on.  In that case, the other data structure will handle the allocations.
/// If the pairing heap is the primary data structure, consider using a regualar
/// binary heap instead.
/// @param [in] key: Pointer to an initializer for the outer struct.  ft->base.size bytes
/// are copied from here to the newly allocated node.  WARNING: ft->base.size is the
/// offset of the cr8r_pheap_node struct within the outer struct, so if there is
/// padding between the used portion of the outer struct and the node struct
/// it contains, or the node struct is not the last member of the outer struct, this
/// function does not work and the allocation must be done manually.
/// @param [in] first_child, sibling, parent: Initializers for the intrusive struct's fields
/// @return A pointer to the outer struct which was allocated and initialized, or NULL if the allocator failed.
void *cr8r_pheap_new(void *key, cr8r_pheap_ft*, cr8r_pheap_node *first_child, cr8r_pheap_node *sibling, cr8r_pheap_node *parent);

/// Return a pointer to the minimal data element of a pairing heap, or NULL if the heap is empty.
/// The pointer returned is to the outer struct.
void *cr8r_pheap_top(cr8r_pheap_node*, cr8r_pheap_ft*);

/// Return a pointer to the root node given a pointer to any node in a heap.
cr8r_pheap_node *cr8r_pheap_root(cr8r_pheap_node*);

/// Combine two pairing heaps, invalidating them
/// The nodes of the existing heaps are reused and re-linked so no new
/// allocations are done, but the original heaps are mutated.
/// @param [in,out] a, b: pointers to root nodes of two existing heaps.
/// @return Returns a pointer to the root element of the melded heap
cr8r_pheap_node *cr8r_pheap_meld(cr8r_pheap_node *a, cr8r_pheap_node *b, cr8r_pheap_ft*);

/// Insert an item into a pairing heap
/// Allocates and initializes a new node for the new item.
/// @param [in] key: Pointer to an initializer for the outer struct.  ft->base.size bytes
/// are copied from here to the newly allocated node.  The same warning from { @link cr8r_pheap_new }
/// applies.
/// @return The new root element on success or NULL on failure.
/// Failure only occurs if allocation fails: items with duplicate keys are permitted with unspecified
/// storage order
cr8r_pheap_node *cr8r_pheap_push(cr8r_pheap_node*, void *key, cr8r_pheap_ft*);

/// Remove the root (minimal) element from a pairing heap.
/// The input pointer is modified to indicate the new root.
/// Does nothing if called on an empty heap.
/// @param [in,out] r: pointer to pointer to root node
/// @return The old root, but its links may not be nulled.
cr8r_pheap_node *cr8r_pheap_pop(cr8r_pheap_node **r, cr8r_pheap_ft*);

/// Restore the (min) heap invariant after decreasing the key of a node.
/// @param [in] n: the node whose key was decreased and may now be less than its parent
/// @return the root of the heap after any necessary changes
cr8r_pheap_node *cr8r_pheap_decreased_key(cr8r_pheap_node *n, cr8r_pheap_ft*);

/// Delete all nodes in a pairing heap.
/// The same note as for cr8r_pheap_new applies here:
/// typically, the allocation should be handled by a primary
/// data structure and the pairing heap should just be a
/// secondary data structure.
void cr8r_pheap_delete(cr8r_pheap_node*, cr8r_pheap_ft*);

