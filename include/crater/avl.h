#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// A featureful generic avl tree implementation.  Useful for storing ordered
/// mappings.
///
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <stddef.h>
#include <stdbool.h>

#include <crater/container.h>
#include <crater/sla.h>

/// An avl tree node, also used to store an entire tree by synecdoche.
/// The data field is a flexible length array in which any element type (uint64_t, custom struct, etc) can be stored.
/// Since avl tree are often used as ordered maps, the thing stored in the data field is often referred to as the "element"
/// of the node because it is an element of the ordered map represented by the tree.  It is also often referred to
/// as the "key" (especially in parameter names) or the "value".  Properly, the "key" is whatever part of the element the
/// comparison function cares about, and the "value" is the rest.  There are a lot of advantages to this approach compared to
/// the usual key, value pair approach, in particular that one struct can be used with many different key functions, such as
/// selecting the x, y, or z coordinate of a point struct.
/// Take care if manipulating these fields directly, this should only
/// be done if the functions in this file are not sufficient
typedef struct cr8r_avl_node cr8r_avl_node;
struct cr8r_avl_node{
	/// Pointers to other nodes (can be NULL)
	cr8r_avl_node *left;
	/// Pointers to other nodes (can be NULL)
	cr8r_avl_node *right;
	/// Pointers to other nodes (can be NULL)
	cr8r_avl_node *parent;
	/// avl balance value; the height of the right subtree minus the height of the left subtree.  Magnitude cannot exceed 1,
	/// providing the avl balance guarantee
	signed char balance;
	/// element data
	char data[];
};

/// Function table for avl tree.
/// Imagine this struct as the class of the hash table.
/// This struct specifies the size of the elements in an avl nodes in bytes,
/// as well as how to perform necessary operations (compare, free, etc).
typedef struct{
	/// Base function table values (data and size)
	cr8r_base_ft base;
	/// Function to compare elements.  Should only depend on "key" data within the elements.
	/// Should return <0 if the first operand is "smaller", 0 if the two operands are "equal", or >0 if the second operand is "smaller".
	int (*cmp)(const cr8r_base_ft*, const void*, const void*);
	/// Function to combine two elements.  Should only depend on "value" data within the elements.
	/// Specifying this function can be used to define behavior when { @link cr8r_avl_insert_update} is called and an element
	/// with the given key already exists, such as adding the int values in any key->int map to create a counter.
	/// The first operand (second argument) is the element in the node already in the tree, the second operand is the element that was attempted to add.
	/// Should return nonzero on success, zero on failure.
	int (*add)(cr8r_base_ft*, void*, void*);
	/// Function to allocate a new node.  This should allocate offsetof(cr8r_avl_node, data) + size bytes.  The slab allocator in this library is designed
	/// to work well for allocating nodes
	void *(*alloc)(cr8r_base_ft*);
	/// Function to deallocate a node.
	void (*free)(cr8r_base_ft*, void*);
} cr8r_avl_ft;

/// Constants to test map like data structure insertion against where applicable
typedef enum{
	CR8R_AVL_FAILED = 0,
	CR8R_AVL_INSERTED = 1,
	CR8R_AVL_UPDATED = 2
} cr8r_map_insert_result;

/// Convenience function to initialize a { @link cr8r_avl_ft }
///
/// Using standard structure initializer syntax with designated initializers may be simpler.
/// However, this function provides basic checking (it checks the required functions aren't NULL).
/// @param [in] data: pointer to user defined data to associate with the function table.
/// generally NULL is sufficient.  see { @link cr8r_base_ft } for a more in-depth explaination
/// @param [in] size: size of a single element in bytes.  Note that the size of a node will be offsetof(cr8r_avl_node, data) + size
/// @param [in] cmp: comparison function.  should not be NULL.  some functions do not require it,
/// but it is required to do anything useful with avl trees.  See { @link cr8r_default_cmp } for a basic generic implementation.
/// @param [in] add: element composition function.  can be NULL for all functions besides { @link cr8r_avl_insert_update }.
/// called to combine an existing and new element when this function is called and the element to insert is already in the tree.
/// @param [in] alloc: allocate a single node.  Using a slab allocator ( { @link cr8r_sla } ) is a good choice.
/// @param [in] free: free a single node.
/// @return 1 on success, 0 on failure (if cmp, alloc, or free is NULL)
bool cr8r_avl_ft_init(cr8r_avl_ft*,
	void *data, uint64_t size,
	int (*cmp)(const cr8r_base_ft*, const void*, const void*),
	int (*add)(cr8r_base_ft*, void*, void*),
	void *(*alloc)(cr8r_base_ft*),
	void (*free)(cr8r_base_ft*, void*)
);

/// Convenience function to initialize a { @link cr8r_avl_ft } and associated slab allocator
///
/// Automatically initializes sla, points ft->base.data at sla, and sets ft->alloc and ft->free to
/// { @link cr8r_default_alloc_sla } and { @link cr8r_default_free_sla } respectively.
/// WARNING sla is always initialized, an initialized sla should not be passed and in particular
/// to make multiple tables with the same slab allocator, call this once and then call { @link cr8r_avl_ft_init }
/// or use a literal for subsequent function tables.
/// @param [out] sla: slab allocator to initialize and point ft->base.data at.  must be uninitialized.
/// it is possible to have more information in ft->base.data by placing sla in a structure and additional information
/// before or after it.
/// @param [in] size: size of a single element (of the avl tree) in bytes.  the size of a node (or slab allocator element)
/// is offsetof(cr8r_avl_node, data) + size.
/// @param [in] reserve: how many nodes to reserve space for in the slab allocator initially.  must not be 0.
/// @param [in] cmp: comparison function.  should not be NULL.  some functions do not require it,
/// but it is required to do anything useful with avl trees.  See { @link cr8r_default_cmp } for a basic generic implementation.
/// @param [in] add: element composition function.  can be NULL for all functions besides { @link cr8r_avl_insert_update }.
/// called to combine an existing and new element when this function is called and the element to insert is already in the tree.
/// @return 1 on success, 0 on failure (if cmp, sla, or reserve is NULL/0 or the slab allocator cannot reserve enough memory)
bool cr8r_avl_ft_initsla(cr8r_avl_ft*,
	cr8r_sla *sla, uint64_t size, uint64_t reserve,
	int (*cmp)(const cr8r_base_ft*, const void*, const void*),
	int (*add)(cr8r_base_ft*, void*, void*)
);

/// Allocate a new avl node and initialize it with given data
///
/// Remember that ft->alloc(ft->base.data) will be called to allocate the node
/// @param [in] key: element to put into the node.  This is generally a structure with conceptual "key" and "value" parts.
/// @param [in] left, right, parent: initial values for node pointers.  This function does NOT adjust the links in these pointers.  Generally left and right will be NULL and changed later.
/// @param [in] balance: avl balance value, generally 0. 
/// @return a pointer to the new, initialized node, or NULL if ft->alloc fails
cr8r_avl_node *cr8r_avl_new(void *key, cr8r_avl_node *left, cr8r_avl_node *right, cr8r_avl_node *parent, char balance, cr8r_avl_ft*);

/// Create a new node in an avl tree with a given value
///
/// @param [in, out] r: root node.  This is a pointer to a pointer, so that if the root node changes, the change can be indicated to the caller.  *r can be NULL to indicate an empty tree.
/// @param [in] key: element to insert.  If no node with an element comparing equal to this element exists in the tree, a new node is created in the tree to hold it.
/// @return 0 if allocation fails or a node with the given element is already present in the tree, 1 if the element is not present and insertion suceeds.
int cr8r_avl_insert(cr8r_avl_node **r, void *key, cr8r_avl_ft*);

/// Remove the node in an avl tree with a given value
///
/// If multiple nodes with the same value exist in the tree,
/// their order is unspecified and an unspecified one will be removed.
/// Notice that { @link cr8r_avl_insert } will never create a tree with duplicate nodes.
/// The removed node is freed using ft->free.
/// @param [in, out] r: root node.  This is a pointer to a pointer, so that if the root node changes, the change can be indicated to the caller.  *r can be NULL to indicate an empty tree.
/// @param [in] key: element to remove.
/// @return 0 if no node with the given element exists in the tree, 1 if successful
int cr8r_avl_remove(cr8r_avl_node **r, void *key, cr8r_avl_ft*);

/// Create a new node in an avl tree or modify an existing one with a given value
///
/// @param [in, out] r: root node.  This is a pointer to a pointer, so that if the root node changes, the change can be indicated to the caller.  *r can be NULL to indicate an empty tree.
/// @param [in] key: element to insert.  If no node with an element comparing equal to this element exists in the tree, a new node is created in the tree to hold it.
/// @return 0 if allocation fails, 1 (AVL_INSERTED) if the element is not present and insertion suceeds, 2 (AVL_UPDATED) if an existing node was updated.
int cr8r_avl_insert_update(cr8r_avl_node **r, void *key, cr8r_avl_ft*);

/// Remove a given node from the tree containing it.
///
/// This modifies the containing tree and frees the removed node.
/// @param [in] n: node to remove.  n->left, n->right, and n->parent are used to find the rest of the tree and rebuild it without this node.  This node is freed.
/// @return a pointer to the new root node of the tree that contained n, in case it changed.  Can be NULL.
cr8r_avl_node *cr8r_avl_remove_node(cr8r_avl_node *n, cr8r_avl_ft*);

/// Add a given node to an existing tree.
///
/// The existing tree may contain a node with the same element (see { @link cr8r_avl_attach_exclusive } for a version that disallows this).
/// @param [in] r: the root of the existing tree.  Notice this is a pointer, not a pointer to a pointer as many other functions have.  Can be NULL.
/// @param [in] n: the node to insert.  Should not have links.
/// @param [out] is_duplicate: if this is not null, 0 is written if the node's key is not equal to any existing node, and 1 is written if it is.
/// @return a pointer to the new root, in case it changed.
cr8r_avl_node *cr8r_avl_attach(cr8r_avl_node *r, cr8r_avl_node *n, cr8r_avl_ft*, int *is_duplicate);

/// Add a given node to an existing tree.
///
/// The existing tree should not already contain a node with the same element.
/// @param [in] r: the root of the existing tree.  Notice this is a pointer, not a pointer to a pointer as many other functions have.  Can be NULL.
/// @param [in] n: the node to insert.  Should not have links.
/// @return a pointer to the new root, in case it changed, or NULL if n has the same key as an existing node
cr8r_avl_node *cr8r_avl_attach_exclusive(cr8r_avl_node *r, cr8r_avl_node *n, cr8r_avl_ft*);

/// Remove a given node from the tree containing it but do not free it.
///
/// This modifies the containing tree and clears the pointers in the node, but allows further use of the node.
/// @param [in, out] n: n->left, n->right, and n->parent are used to find the rest of the tree and rebuild it without this node, then cleared so this node is a singleton.
/// @return a pointer to the new root of the tree that contained the node, in case its root changed.  Can be NULL if the last node was removed.
cr8r_avl_node *cr8r_avl_detach(cr8r_avl_node *n, cr8r_avl_ft*);

/// Find the node matching a given element in a tree
///
/// @param [in] r: the root of the tree to search
/// @param [in] key: element to find in the tree
/// @return a pointer to the node matching the given key, or NULL if no match exists.
cr8r_avl_node *cr8r_avl_get(cr8r_avl_node *r, void *key, cr8r_avl_ft*);

/// Find the deepest node on the search path for a given key
/// If there is a node in the tree with the given key, returns a pointer to that node.
/// Otherwise, returns a pointer leaf node which is either the lower bound or upper bound
/// of the given key.  In both cases, the node returned is the deepest node that would
/// be an ancestor of the key if it were inserted into the tree without rebalancing.
///
/// @param [in] r: the root of the tree to search
/// @param [in] key: element to search for in the tree
/// @return a pointer to the last node in the tree on the search path for the given key,
/// which could compare equal to the key, be its lower bound or upper bound, or, if the given
/// root is null, be null
cr8r_avl_node *cr8r_avl_search(cr8r_avl_node *r, void *key, cr8r_avl_ft*);

/// Find the root of the tree containing a given node.
///
/// Simply climbs the parent links repeatedly.
/// @param [in] n: node in tree to find root of.
/// @return a pointer to the root of the tree containing n.
cr8r_avl_node *cr8r_avl_root(cr8r_avl_node *n);

/// Find the node in the tree with the lowest key
///
/// Simply follows the left links repeatedly.
/// @param [in] r: the root of the tree
/// @return a pointer to the node with the lowest key in the tree
cr8r_avl_node *cr8r_avl_first(cr8r_avl_node *r);

/// Find the inorder successor of a node
///
/// @param [in] n: node to find the successor of
/// @return a pointer to the inorder successor of n
cr8r_avl_node *cr8r_avl_next(cr8r_avl_node *n);

/// Find the node in the tree with the greatest key
///
/// Simply follows the right links repeatedly.
/// @param [in] n: the root of the tree
/// @return a pointer to the node with the greatest key in the tree
cr8r_avl_node *cr8r_avl_last(cr8r_avl_node *n);

/// Find the inorder predecessor of a node
///
/// @param [in] n: node to find the predecessor of
/// @return a pointer to the inorder predecessor of n
cr8r_avl_node *cr8r_avl_prev(cr8r_avl_node *n);

/// Find the greatest element l in the tree so that l <= key
///
/// This is useful along with { @link cr8r_avl_upper_bound } for partitioning the tree into ranges,
/// and partitions the inorder traversal in a nice way.  In particular, if the tree has no
/// duplicate keys, then cr8r_avl_upper_bound is the inorder successor of cr8r_avl_lower_bound, so
/// if we start at { @link cr8r_avl_first } or some node known to have a key less than the key given to cr8r_avl_lower_bound,
/// then we can do an inorder traversal with { @link cr8r_avl_next } from the starting point to the lower bound (inclusive).
/// @param [in] r: root of the tree to search
/// @param [in] key: element to find a maximal inclusive lower bound of in the tree
/// @return a pointer to a node whose key is a maximal inclusive lower bound
cr8r_avl_node *cr8r_avl_lower_bound(cr8r_avl_node *r, void *key, cr8r_avl_ft*);

/// Find the lowest element u in the tree so that key < u
///
/// See { @link cr8r_avl_lower_bound }
/// @param [in] r: root of the tree to search
/// @param [in] key: element to find a minimal exclusive upper bound of in the tree
/// @return a pointer to a node whose key is a minimal exclusive upper bound
cr8r_avl_node *cr8r_avl_upper_bound(cr8r_avl_node *r, void *key, cr8r_avl_ft*);

/// Delete an entire avl tree, freeing all nodes in the process
///
/// @param [in] r: the root of the tree, which is invalidated (unless ft->free doesn't invalidate nodes for some reason)
void cr8r_avl_delete(cr8r_avl_node *r, cr8r_avl_ft*);

/// Restore the binary search tree invariant after decreasing the key for a single node
///
/// After decreasing the key in a node, this function should be called, which then moves the node
/// if necessary to ensure the tree is a BST.  Only the given node should violate the BST condition.
/// @param [in] n: the node that has had its key decreased and may need to be moved
/// @param [out] is_duplicate: if this is not null, 0 is written if the decreased key is unique within the tree,
/// and 1 is written if it is equal to some existing key
/// @return a pointer to the root of the tree, in case it changes
cr8r_avl_node *cr8r_avl_decrease(cr8r_avl_node *n, cr8r_avl_ft*, int *is_duplicate);

/// Restore the binary search tree invariant after increasing the key for a single node
///
/// After increasing the key in a node, this function should be called, which then moves the node
/// if necessary to ensure the tree is a BST.  Only the given node should violate the BST condition.
/// @param [in] n: the node that has had its key increased and may need to be moved
/// @param [out] is_duplicate: if this is not null, 0 is written if the increased key is unique within the tree,
/// and 1 is written if it is equal to some existing key
/// @return a pointer to the root of the tree, in case it changes, or NULL if a duplicate key is encountered
cr8r_avl_node *cr8r_avl_increase(cr8r_avl_node *n, cr8r_avl_ft*, int *is_duplicate);

/// Find the first node in a postorder traversal of the tree
///
/// A postorder traversal visits the children of a node before the node,
/// useful for evaluating expression trees and so on.
/// @param [in] r: pointer to the root
/// @return a pointer to the first node in a postorder traversal
cr8r_avl_node *cr8r_avl_first_post(cr8r_avl_node *r);

/// Find the postorder successor of a given node
///
/// @param [in] n: current node
/// @return a pointer to the next node in a postorder traversal
cr8r_avl_node *cr8r_avl_next_post(cr8r_avl_node *n);

/// Treat the avl tree as a max heap and sift down a given node
///
/// If this node has had its key decreased but the avl tree is otherwise a max heap,
/// this will restore the max heap invariant.
/// @param [in] r: the node to sift down
void cr8r_avl_sift_down(cr8r_avl_node *r, cr8r_avl_ft*); 

/// Convert the avl tree into a max heap in place
///
/// This takes linear time.  The shape of the tree is not changed.
/// To change the ordering function of the heap, this function can just be called again
/// on a formed heap with ft->cmp changed appropriately.
/// @param [in, out] r: root of the tree to heapify
void cr8r_avl_heapify(cr8r_avl_node *r, cr8r_avl_ft*);

/// Treat the avl tree as a max heap and remove the top element
///
/// Restores heap invariant afterwards
/// @param [in, out] r: pointer to pointer to root node, updated to point to new root node afterwards
/// @return pointer to removed node, or NULL if *r was NULL
cr8r_avl_node *cr8r_avl_heappop_node(cr8r_avl_node **r, cr8r_avl_ft*);

/// Reorder the tree so that it is sorted according to a new ordering function
///
/// Currently this works by heapifying the tree in place and then placing the nodes in order one by one,
/// which takes n*log(n) time.
/// @param [in, out] r: root of the tree to reorder, reordering is in place and does not change the root
void cr8r_avl_reorder(cr8r_avl_node *r, cr8r_avl_ft*);

/// Get a pointer to the data field of an avl node and cast to a given type
#define CR8R_AVL_DATA(T, n) CR8R_FLA_CAST(T, (char*)((cr8r_avl_node*)(n))->data)

