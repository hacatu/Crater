#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// Simple generic singly linked list
/// Remember that this is all fun and games, but a vector will
/// generally be much faster for just about everything.
///
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <crater/container.h>
#include <crater/sla.h>

/// A circular linked list node, or an entire circular linked list by synecdoche
/// Note that a pointer to the LAST node is typically passed around, because last->next
/// gets the first node.
/// Take care if manipulating these fields directly, this should only
/// be done if the functions in this file are not sufficient
typedef struct cr8r_cll_node cr8r_cll_node;
struct cr8r_cll_node{
	/// pointer to the next node in the list
	cr8r_cll_node *next;
	/// flexible length array holding element
	char data[];
};

/// Function table for linked list.
/// Imagine this struct as the class of the linked list.
/// This struct specifies the size of the elements in a list in bytes,
/// as well as how to perform necessary operations (compare, copy, etc).
/// Remember that function tables must be initialized manually.  In particular,
/// the cr8r_vec_default_* functions can be used as decent defaults, but must be explicitly set.
typedef struct{
	/// Base function table values (data and size)
	cr8r_base_ft base;
	/// allocate a new node.  should allocate offsetof(cr8r_cll_node, data) + data->size bytes.
	/// the slab allocator in this library is a good option for this.
	void *(*alloc)(cr8r_base_ft*);
	/// deallocate a node, first cleaning up anything owned by its element if necessary.
	/// to be safe, the allocation scheme should either zero out allocated memory before
	/// returning it OR not have elements own anything, since if neither of these things
	/// is done it's possible for this function to try to clean up a pointer in uninitialized
	/// memory, which is Very Bad.
	void (*del)(cr8r_base_ft*, void *p);
	/// copy an element of the vector.  Can be NULL if memcpy is sufficient, otherwise this function must perform the role of memcpy
	/// plus whatever else it needs to do.
	void (*copy)(cr8r_base_ft*, void *dest, const void *src);
	/// compare two elements of the vector.  { @link cr8r_default_cmp } is a suitable default if needed.
	/// should return <0 if a < b, 0 if a == b, and >0 if a > b.  should not return INT_MIN.
	int (*cmp)(const cr8r_base_ft*, const void *a, const void *b);
} cr8r_cll_ft;


/// Convenience function to initialize a { @link cr8r_cll_ft }
///
/// Using standard structure initializer syntax with designated initializers may be simpler.
/// However, this function provides basic checking (it checks the required functions aren't NULL).
/// @param [in] data: pointer to user defined data to associate with the function table.
/// generally NULL is sufficient.  see { @link cr8r_base_ft } for a more in-depth explaination
/// @param [in] size: size of a single element in bytes.  Note that the size of a node will be offsetof(cr8r_cll_node, data) + size
/// @param [in] alloc: allocate a single node.  Using a slab allocator ( { @link cr8r_sla } ) is a good choice.
/// @param [in] del: free a single node.
/// @param [in] copy: copy an element.  can be NULL if memcpy is sufficient.
/// @param [in] cmp: comparison function.  must not be NULL in order to call any searching or sorting functions
/// @return 1 on success, 0 on failure (if alloc or del is NULL)
bool cr8r_cll_ft_init(cr8r_cll_ft*,
	void *data, uint64_t size,
	void *(*alloc)(cr8r_base_ft*),
	void (*del)(cr8r_base_ft*, void*),
	void (*copy)(cr8r_base_ft*, void *dest, const void *src),
	int (*cmp)(const cr8r_base_ft*, const void*, const void*)
);

/// Convenience function to initialize a { @link cr8r_cll_ft } and associated slab allocator
///
/// Automatically initializes sla, points ft->base.data at sla, and sets ft->alloc and ft->free to
/// { @link cr8r_default_alloc_sla } and { @link cr8r_default_free_sla } respectively.
/// WARNING sla is always initialized, an initialized sla should not be passed and in particular
/// to make multiple tables with the same slab allocator, call this once and then call { @link cr8r_cll_ft_init }
/// or use a literal for subsequent function tables.
/// @param [out] sla: slab allocator to initialize and point ft->base.data at.  must be uninitialized.
/// it is possible to have more information in ft->base.data by placing sla in a structure and additional information
/// before or after it.
/// @param [in] size: size of a single element (of the circular list) in bytes.  the size of a node (or slab allocator element)
/// is offsetof(cr8r_cll_node, data) + size.
/// @param [in] reserve: how many nodes to reserve space for in the slab allocator initially.  must not be 0.
/// @param [in] copy: copy an element.  can be NULL if memcpy is sufficient.
/// @param [in] cmp: comparison function.  should not be NULL.  some functions do not require it,
/// but it is required to do anything useful with avl trees.  See { @link cr8r_default_cmp } for a basic generic implementation.
/// @return 1 on success, 0 on failure (if cmp, sla, or reserve is NULL/0 or the slab allocator cannot reserve enough memory)
bool cr8r_cll_ft_initsla(cr8r_cll_ft*,
	cr8r_sla *sla, uint64_t size, uint64_t reserve,
	void (*copy)(cr8r_base_ft*, void *dest, const void *src),
	int (*cmp)(const cr8r_base_ft*, const void*, const void*)
);


/// Allocate a new list node and initialize it with given data
///
/// The next field in the created node points to itself, as a singleton circular linked list.
/// @param [in] data: element to put into the node.  This is generally a structure with conceptual "key" and "value" parts.
/// @return a pointer to the new, initialized node, or NULL if ft->alloc fails.
cr8r_cll_node *cr8r_cll_new(cr8r_cll_ft*, const void *data);

/// Delete a list
///
/// Calls ft->del on each node
void cr8r_cll_delete(cr8r_cll_node*, cr8r_cll_ft*);

/// Create a copy of a list
///
/// Copies entries with ft->copy if applicable.
/// @return pointer to a copy of the list on success, or NULL if ft->alloc fails.
cr8r_cll_node *cr8r_cll_copy(const cr8r_cll_node*, cr8r_cll_ft*);

/// Create a list from an array
///
/// Copies entries with ft->copy if applicable.
/// @param [in] len: length of the array
/// @param [in] a: pointer to the beginning of the array
/// @return pointer to a list copied from the array on success, or NULL if ft->alloc fails.
cr8r_cll_node *cr8r_cll_from(cr8r_cll_ft*, uint64_t len, const void *a);


/// Add a node with a given value at the beginning of the list
///
/// This is an O(1) operation.
/// @param [in, out] self: pointer to pointer to last node in current list, so that if the last node changes (ie if inserting into a NULL list) it can be updated
/// @param [in] val: value to add to the list.  ft->alloc is called to create the node to place it in
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_cll_pushl(cr8r_cll_node **self, cr8r_cll_ft*, const void *val);

/// Remove the first node in the list
///
/// This is an O(1) operation.
/// @param [in, out] self: pointer to pointer to last node in current list, so that if the last node changes (ie if removing from a singleton list) it can be updated
/// @param [out] out: the element of the removed node is copied here and then the removed node is deallocated with ft->del
/// @return 1 on success, 0 on failure (if *self is NULL corresponding to removing from a NULL list)
bool cr8r_cll_popl(cr8r_cll_node **self, cr8r_cll_ft*, void *out);

/// Add a node with a given value at the end of the list
///
/// This is an O(1) operation.
/// @param [in, out] self: pointer to pointer to last node in current list, so it can be updated because this operation always changes it to the new node
/// @param [in] val: value to add to the list.  ft->alloc is called to create the node to place it in
/// @return 1 on success, 0 on failure (allocation failure)
bool cr8r_cll_pushr(cr8r_cll_node **self, cr8r_cll_ft*, const void *val);

/// Remove the first node in the list
///
/// This is an O(n) operation.  Using a circular linked list allows us to go from having just pushl and popl be constant time to pushr as well, but to get constant time
/// popr a doubly linked list (including XOR or offset lists) is needed.
/// @param [in, out] self: pointer to pointer to last node in current list, so that it can be updated because this operation always changes it to the previous node
/// unless the list is a single or empty, in which case this is set to NULL or the operation fails, respectively.
/// @param [out] out: the element of the removed node is copied here and then the removed node is deallocated with ft->del
/// @return 1 on success, 0 on failure (if *self is NULL corresponding to removing from a NULL list)
bool cr8r_cll_popr(cr8r_cll_node **self, cr8r_cll_ft*, void *out);


/// Create a new list from the subsequence of a given list satisfying a predicate
///
/// @param [out] dest: pointer to store filtered list in
/// @param [in] src: list to copy from
/// @param [in] pred: predicate function, called on all elements of the input list.  elements for which 1 is returned are copied to the output.
/// @return 1 on success, or 0 on allocation failure.
bool cr8r_cll_filtered(cr8r_cll_node **dest, const cr8r_cll_node *src, cr8r_cll_ft*, bool (*pred)(const void*));

/// Filter a list in place by deleting all nodes that don't match a predicate
///
/// @param [in, out] self: pointer to pointer to last element of list to filter, so that it can be updated if it changes
/// @param [in] pred: predicate function, called on all elements of the input list.  elements for which 0 is returned are removed and deleted.
void cr8r_cll_filter(cr8r_cll_node **self, cr8r_cll_ft*, bool (*pred)(const void*));

/// Create a new list by applying a transformation function to every element in a list
///
/// @param [out] dest: pointer to store map output list in
/// @param [in] src: list to map from
/// @param [in] dest_ft: function table { @link cr8r_cll_ft }
/// @param [in] f: transformation function, called on all elements of the input list to produce elements of output list.
/// the first argument is a pointer to the output list element to populate, and the second argument is a pointer to the input list
/// element to map from.
/// @return 1 on success, 0 on allocation failure
bool cr8r_cll_mapped(cr8r_cll_node **dest, const cr8r_cll_node *src, cr8r_cll_ft *dest_ft, void (*f)(void *o, const void *e));

/// Execute a given function on each element in a list
///
/// Processes elements in order starting with the first
/// Only useful for dirty, dirty side effects
/// @param [in] f: callback function
void cr8r_cll_forEach(cr8r_cll_node*, cr8r_cll_ft*, void (*f)(void*));

/// Stitch together two lists, one after the other
///
/// Given two lists, connects them to form a longer list.
/// Updates a->next, b->next = b, a->next and returns b.
/// @param [in, out] a: pointer to the last element of the first list
/// @param [in, out] b: pointer to the last element of the second list
/// @return pointer to the last element of the combined list, namely b
cr8r_cll_node *cr8r_cll_combine(cr8r_cll_node *a, cr8r_cll_node *b, cr8r_cll_ft*);


/// Test if a predicate holds for all elements in a list
///
/// Returns false immediately if any element does not satisfy the predicate.
/// @param [in] pred: predicate function, called on each element in the list
/// @return 1 if the predicate function returns 1 on all elements, 0 (as soon as it doesn't) otherwise
bool cr8r_cll_all(const cr8r_cll_node*, cr8r_cll_ft*, bool (*pred)(const void*));

/// Test if a predicate holds for any element in a list
///
/// Returns true immediately if any element satisfies the predicate.
/// Note that this is completely equivalent to the negation of { @link cr8r_cll_all }
/// with the negation of the predicate.
/// @param [in] pred: predicate function, called on each element in the list
/// @return 1 (as soon as) the predicate function returns 1 on any element, 0 if it returns 0 on all of them
bool cr8r_cll_any(const cr8r_cll_node*, cr8r_cll_ft*, bool (*pred)(const void*));


/// Find the first node in a list with an element matching a given element according to ft->cmp
///
/// This is O(n) because it simply does a linear search.
/// @param [in] e: element to search for (using ft->cmp)
/// @return pointer to the first node matching the given element, or NULL if none matches
cr8r_cll_node *cr8r_cll_lsearch(cr8r_cll_node*, cr8r_cll_ft*, const void *e);

/// Perform a right fold on a list
///
/// Given an initial accumulator value of some type T in a void pointer, a list, and a function f that takes a void pointer to
/// a T value and an element of the list and returns a void pointer to a T value, this function repeatedly applies
/// f to the current accumulator and element to get the new accumulator value.
/// This is repeated for every element in the left, going from left to right, and the final accumulator value is returned.
/// The accumulation function f must handle freeing old accumulator values once it is finished with them if necessary.
/// @param [in] init: starting value for the accumulator
/// @param [in] f: accumulation function: should take current accumulator value (as void pointer) and current list element
/// and return new accumulator value.  It should either modify the accumulator value in place, returning it as well,
/// or allocate a new pointer for the new accumulator value and free the old one once the new one has been computed.  In cases
/// where the accumulator value is an integer or something else that fits within a pointer, the pointer can be used as a casted
/// integer or whatnot instead of as a pointer.
/// @return the final accumulator value
void *cr8r_cll_foldr(const cr8r_cll_node*, cr8r_cll_ft*, void *init, void *(*f)(void *acc, const void *e));


/// Sort a list in place
///
/// Uses a merge sort like algorithm
/// @param [in, out] self: pointer to pointer to last node in list.  updated to contain a pointer to the new last node in case it changes
void cr8r_cll_sort(cr8r_cll_node **self, cr8r_cll_ft*);

/// Create a sorted copy of a list
///
/// Simply calls { @link cr8r_cll_copy } and then { @link cr8r_cll_sort } on the copy
/// @return pointer to the last element of the sorted copy, or NULL on an allocation failure
/// Note that NULL is also returned if the input list is empty, but these reasons for returning NULL are mutually exclusive
cr8r_cll_node *cr8r_cll_sorted(const cr8r_cll_node*, cr8r_cll_ft*);

/// Reverse a list in place
///
/// @param [in, out] self: the list to reverse.  passed as a pointer to a pointer to the last node, so that the pointer to the last node can be updated
void cr8r_cll_reverse(cr8r_cll_node **self, cr8r_cll_ft*);

/// Creates a reversed copy of a list
///
/// @return pointer to the last element of the reversed copy, or NULL on an allocation failure
/// Note that NULL is also returned if the input list is empty, but these reasons for returning NULL are mutually exclusive
cr8r_cll_node *cr8r_cll_reversed(const cr8r_cll_node*, cr8r_cll_ft*);

