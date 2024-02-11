#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// A fast hash table using quadratic probing and an incremental split table
/// for growing.  If the table needs to be extended, a second internal table
/// will be created. All new entries will be placed in the second table,
/// and all hash table operations will move one entry from the old table
/// to the new table, to amortize the cost.
///
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>

#include <crater/container.h>

/// A hash table.
/// Fields of this struct should not be edited directly, only through the functions in this file.
typedef struct{
	/// Buffer for the main internal table
	void *table_a;
	/// Buffer for the second internal table if present
	void *table_b;
	/// Metadata about which entries are present/deleted/absent in the main table.
	/// Two bits per entry: present bit and deleted bit.
	uint64_t *flags_a;
	/// Metadata about which entries are present/deleted/absent in the second table.
	uint64_t *flags_b;
	/// Total number of elements that can be stored before expanding the table.
	/// Recomputed only when the table actually expands.  In particular, inserting
	/// multiple elements with different load factors in the function table will not
	/// cause a re-hash ({ @link cr8r_hashtbl_ft::load_factor}, { @link cr8r_hash_insert }).
	uint64_t cap;
	/// Length of buffer for main internal table (in elements not bytes).
	uint64_t len_a;
	/// Length of buffer for second internal table (in elements not bytes).
	uint64_t len_b;
	/// Current index for incremental rehashing
	uint64_t i;
	/// Amount of entries that are rehashed at once.
	/// Like { @link cr8r_hashtbl_t::cap}, this is only updated when the table expands
	/// (or incrementally moving entries from the old table to the new table finishes).
	uint64_t r;
	/// Total number of elements currently in the table.
	uint64_t full;
} cr8r_hashtbl_t;

/// Function table for hash table.
/// Imagine this struct as the class of the hash table.
/// This struct specifies the size of the elements in a hash table in bytes and maximum load factor,
/// as well as how to perform necessary operations (hash, compare).
typedef struct{
	/// Base function table values (data and size)
	cr8r_base_ft base;
	/// Function to hash an element.  Should only depend on "key" data within the element.
	uint64_t (*hash)(const cr8r_base_ft*, const void*);
	/// Function to compare elements.  Should only depend on "key" data within the elements.
	/// Should return 0 for elements that are equal, or any nonzero int for elements that are not equal.
	/// Equality according to this function should imply equal hash values.
	/// Currently no requirement is placed on return values for unequal operands beyond being nonzero,
	/// but if possible <0 should indicate the first operand is smaller, >0 should indicate the first argument is large,
	/// and the function should avoid returning INT_MIN
	int (*cmp)(const cr8r_base_ft*, const void*, const void*);
	/// Function to combine two elements.  Should only depend on "value" data within the elements.
	/// Specifying this function can be used to define behavior when { @link cr8r_hash_append} is called and an element
	/// with the given key already exists, such as adding the int values in any key->int map to create a counter.
	/// The first void* argument is the existing entry, and the second is the entry passed to append.
	/// Should return 0 on failure, nonzero on success.
	int (*add)(cr8r_base_ft*, void*, void*);
	/// Function to be called to delete elements.  Not required.
	/// Specifying this function allows cleaning up sensitive data by jumbling the memory of an element when it
	/// is deleted (including when the whole hash table is deleted), freeing resources "owned" by elements, and so on.
	/// If not specified, deleting elements/the whole hash table will be slightly quicker.
	void (*del)(cr8r_base_ft*, void*);
	/// Maximum allowable ratio of entries present to total capacity.
	/// According to the birthday paradox, at least one collision is expected even with a perfect hash function and
	/// random data once the load facter reaches the square root of the capacity, but nevertheless good performance
	/// can be expected even up to 50% load factor or possibly higher.  Setting lower than .3 would be extravagant and
	/// lower than .1 would probably be absurd.
	double load_factor;
} cr8r_hashtbl_ft;


/// Convenience function to initialize a { @link cr8r_hashtbl_ft }
///
/// Using standard structure initializer syntax with designated initializers may be simpler.
/// However, this function provides basic checking (it checks the required functions aren't NULL).
/// @param [in] data: pointer to user defined data to associate with the function table.
/// generally NULL is sufficient.  see { @link cr8r_base_ft } for a more in-depth explaination
/// @param [in] size: size of a single element in bytes
/// @param [in] hash: hash function.  should not be NULL.  See { @link cr8r_default_hash_u64 } for a basic uint64_t implementation.
/// @param [in] cmp: comparison function.  should not be NULL.  See { @link cr8r_default_cmp } for a basic generic implementation.
/// @param [in] add: element composition function.  can be NULL for all functions besides { @link cr8r_hash_append }.
/// called to combine an existing and new element when this function is called and the element to insert is already in the tree.
/// @param [in] del: called on any element before deleting it.  can be NULL if no action is required.
/// @return 1 on success, 0 on failure (if hash or cmp is NULL)
bool cr8r_hash_ft_init(cr8r_hashtbl_ft*,
	void *data, uint64_t size,
	uint64_t (*hash)(const cr8r_base_ft*, const void*),
	int (*cmp)(const cr8r_base_ft*, const void*, const void*),
	int (*add)(cr8r_base_ft*, void*, void*),
	void (*del)(cr8r_base_ft*, void*)
);


/// Initialize a hash table.
///
/// The { @link cr8r_hashtbl_ft} argument should be configured manually.
/// This is not likely to change.
/// @param [in] reserve: how many entries to reserve space for.  This is rounded up to a power of 2 and then DOWN to a prime.
/// @return 1 on success, 0 on failure
int cr8r_hash_init(cr8r_hashtbl_t*, const cr8r_hashtbl_ft*, uint64_t reserve);

/// Get a pointer to an entry in the hash table.
///
/// This pointer could be invalidated if a new element is inserted into the hash table, or if any operation is performed while
/// the secondary table is present.  Thus the entry should be copied if it is needed after additional operations.
/// If the hash table should be available from multiple threads, an external lock should be used.
/// @param [in] key: key to search the hash table for
/// @return a pointer to the element if found (which could be invalidated by another hash table operation) or NULL if absent
void *cr8r_hash_get(cr8r_hashtbl_t*, const cr8r_hashtbl_ft*, const void *key);

/// Insert an element into the hash table.
///
/// The element is copied from the pointer provided, so keep in mind both the "key" and "value" components of it should be
/// initialized.  Returns a pointer to the inserted element.  As with { @link cr8r_hash_get}, this pointer could be invalidated
/// if another hash table operation is called, and the hash table should be externally locked if the hash table should
/// be available from multiple threads.  This function will not modify the hash table if an entry with the same key
/// already exists, but a pointer to the existing element will be returned.  { @link cr8r_hash_append} can be used to change the
/// behavior for existing keys.  The status reported can be used to differentiate between the element being inserted and
/// already being present.  If the additional entry would bring the total number of entries over the capacity, the table expands
/// with a second internal buffer.  The capacity is recalculated according to the load factor supplied to this function, the number
/// of elements to move per incremental rehash is recomputed (it is always 1 or 2 currently), and any future calls to most hash
/// table operations will move this many elements from the old table to the new table until it can be free'd.
/// @param [in] key: element to insert.  "key" and "value" components should be initialized.
/// @param [out] status: if not NULL, this pointer is set to 1 on success, 2 if an element with the same key is present already, and 0 on allocation failure
/// @return a pointer to an element with the same key if one exists, otherwise a pointer to the element inserted into the hash table, as if { @link cr8r_hash_get} were called atomically afterwards
void *cr8r_hash_insert(cr8r_hashtbl_t*, const cr8r_hashtbl_ft*, const void *key, int *status);

/// Insert an element into the hash table or modify its value.
///
/// Similar to { @link cr8r_hash_insert} except that if an element with the same key is present already, { @link cr8r_hashtbl_ft::add} is called to allow the existing element and even potentially
/// the element pointed to by key to be modified.  The latter is useful if elements "own" some resource, for example freeing a string in key after appending it to the string in
/// the existing element.  This function requires the add function to be specified.
/// @param [in,out] key: element to insert or modify existing element with.
/// "key" and "value" components should be initialized.  Can be modified if an element with the same key exists and the add function modifies its second argument.
/// @param [out] status: if not NULL, this pointer is set to 1 on success, 2 if an element with the same key is present already, and 0 on allocation failure
/// @return a pointer to an element with the same key if one exists, otherwise a pointer to the element inserted into the hash table, as if { @link cr8r_hash_get} were called atomically afterwards
void *cr8r_hash_append(cr8r_hashtbl_t*, cr8r_hashtbl_ft*, void *key, int *status);

/// Remove the element with a given key.
///
/// Equivalent to { @link cr8r_hash_get} followed by { @link cr8r_hash_delete}.  Sets the deleted bit in the hash table, although this bit is
/// currently unused.
/// @param [in] key: "key" of the element to remove
/// @return 1 if the element is found (and removed), 0 if the element is not found
int cr8r_hash_remove(cr8r_hashtbl_t*, cr8r_hashtbl_ft*, const void *key);

/// Remove an element of the hash table by pointer.
///
/// Useful if the element has already been found via { @link cr8r_hash_get} or { @link cr8r_hash_next}.
/// Sets the deleted bit, although this is currently unused.
/// Does NOT incrementally move entries from the old internal table.
/// @param [in,out] ent: the element to delete
void cr8r_hash_delete(cr8r_hashtbl_t*, cr8r_hashtbl_ft*, void *ent);

/// Remove all entries from the hash table.
///
/// { @link cr8r_hashtbl_ft::del} is called on every entry if specified, otherwise this is
/// constant time assuming free is.  Does not set the unused deleted bit unless a delete function is specified.  Frees
/// the older, smaller internal table if two are present, which will also cause incremental moving to stop the next time it would occur.
void cr8r_hash_clear(cr8r_hashtbl_t*, cr8r_hashtbl_ft*);

/// Free the resources held by the hashtable.
///
/// If { @link cr8r_hashtbl_ft::del} is specified, it is called on every entry before free is called, otherwise, this function is
/// constant time if free is.
void cr8r_hash_destroy(cr8r_hashtbl_t*, cr8r_hashtbl_ft*);

/// Iterate through the entries of a hash table.
///
/// If cur is NULL, find the first entry.  Otherwise, find the next entry.  If no more entries exist (including when cur is NULL
/// but there are no entries), return NULL.  This function does NOT perform incremental moving from the old table.
/// The reason this function and { @link cr8r_hash_delete} do not perform incremental moving is so that iteration and even iteration
/// with deletion can be performed without invalidating pointers to elements of the hash table.  Note that if any of the other hash
/// table operations are called or delete is called from another thread, iteration may fail anyway.  It is safe to delete any entry
/// in the hash table while iterating though, even the current one or ones that have not been visited.  Thus, iteration and deletion
/// may be done in multiple threads at once, so long as each call to this function and delete are guarded with appropriate locks and
/// no other functions are called.  Currently, there is no obvious way to get well distributed pointers to elements to assist multi
/// threaded iteration, but picking pointers at even multiples of element size would work.  That is, add up { @link cr8r_hashtbl_t::len_a}
/// and { @link cr8r_hashtbl_t::len_b}, divide by the number of threads, and use that as the offset between pointers.
/// This function simply scans through the metadata to find the first occupied entry after a given pointer, so passing any pointer
/// into the buffer that is aligned to the element boundaries like that is acceptable.  Each metadata entry describes 32 entries in
/// the hash table, so when the hash table is less than 1/32 full there will be a lot of extraneous metadata reads but for a well
/// filled hash table most metadata reads will successfully find the next occupied entry, and with only 2 bits of overhead per entry rather
/// than 64 bits of overhead per entry in a linked list scheme (or only 1 bit if the unused deleted bit is removed).
/// Obviously this means iteration is not in a predictable order, unlike some
/// hash tables where iteration is in insertion order.
/// @param [in] cur: pointer to the current element, or any poiner into the buffer(s) in the hash table.  Can be NULL.
/// @return pointer to the next element after the supplied pointer.  If the supplied pointer is NULL, return the a pointer to the first element.
/// If there is no next element (or first element), return NULL
inline static void *cr8r_hash_next(cr8r_hashtbl_t *self, const cr8r_hashtbl_ft *ft, void *cur){
	uint64_t a = 0, b = 0;
	if(self->table_b){
		if(cur){
			b = (cur - self->table_b)/ft->base.size;
			if(b++ >= self->len_b){// also goes to SEARCH_A if "b < 0" due to unsigned overflow wrapping
				goto SEARCH_A;
			}
		}
		for(; b < self->len_b; ++b){
			if((self->flags_b[b >> 5] >> (b&0x1F))&0x100000000ULL){
				return self->table_b + b*ft->base.size;
			}
		}
	}else{
		SEARCH_A:
		if(cur){
			a = (cur - self->table_a)/ft->base.size;
			if(a++ >= self->len_a){
				return NULL;
			}
		}
	}
	for(; a < self->len_a; ++a){
		if((self->flags_a[a >> 5] >> (a&0x1F))&0x100000000ULL){
			return self->table_a + a*ft->base.size;
		}
	}
	return NULL;
}

/// arbitrary large prime
extern const uint64_t cr8r_hash_u64_prime;

/// function table for using hash table as set of uint64_t's
extern cr8r_hashtbl_ft cr8r_htft_u64_void;

/// function table for using hash table as map rom uint64_t's to uint64_t's
extern cr8r_hashtbl_ft cr8r_htft_u64_u64;

/// entry type for cstr -> uint64_t mapping
typedef struct{
	/// key
	const char* str;
	/// value
	uint64_t n;
} cr8r_htent_cstr_u64;

/// function table for using hash table as map from c strings to uint64_t's
extern cr8r_hashtbl_ft cr8r_htft_cstr_u64;

