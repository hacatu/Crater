#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// @section LICENSE
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.
/// @section DESCRIPTION
/// KD trees are a way to store spatially organized data (tagged points
/// on a sphere, tagged points in a 2D rectangle, tagged points in
/// a 3D cuboid, etc).

#include "crater/container.h"
#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>

#include <crater/vec.h>

/// Function table for kd tree
///
/// A kd tree is stored as an implicit binary tree in a vector.  The root of the tree is
/// the element of the vector at the middle position, and the left and right subranges
/// correstpond to subtrees with roots at their middles and so on.
/// This means kd trees do NOT support efficient insertion/removal of points.
/// For that, we would need to use a self balancing tree like a modified avl or red black tree.
/// Implicit binary trees are maximally balanced and do not have the space or time overheads
/// of child pointers, so they are the better choice when adding/removing points is rare enough
/// that performing linear time re-treeifying is acceptable.
typedef struct _cr8r_kd_ft cr8r_kd_ft;
/// Function table for kd tree, see { @link cr8r_kd_ft }
struct _cr8r_kd_ft{
	/// Subtable for vector ft
	///
	/// Since kd trees are stored as vectors, a vector ft is included in the kd ft
	/// so that vector functions may be used and vector len/elem size queried without carrying around
	/// two fts.  Note that this is exactly like saying kd extends vec if we consider how fts
	/// are just an implementation of classes in C, hence the name super.
	cr8r_vec_ft super;
	/// Dimension of the kd tree
	///
	/// This is the number of directions points are split in.  For instance, in a kd cuboid tree,
	/// there are k directions to split in.  However, for a sphere in k dimensions, we can split in
	/// only k-1 directions.  The first k-2 simply split points along some axis.  For the final
	/// one however, points are split based on their angle about the origin in some plane.
	/// The important takeaway about this field is that points in the tree might have more than dim coordinates.
	/// To be clear, this is because for a spherical kd tree in k dimensions, only points on the surface of the
	/// sphere are allowed, or to be more accurate, all points are effectively projected onto the surface of
	/// a (hyper)cylinder with the same axis (axes).  These peculiarities are specific to the implementation of
	/// { @link cr8r_kdft_s2i64 } and one could easily create a spherical kd tree that resolves these issues in
	/// other ways.
	uint64_t dim;
	/// Size of a bounds object
	///
	/// Generally a bounds object is encoded as bl, tr, where bl is a point with each coordinate equal to the
	/// minimum of that coordinate over all points in the tree, and tr is the same but for maximum (standing for
	/// bottom left and top right).
	/// TODO: should we replace bounds_size with coords or something?  ft->super.base.size is the size of the
	/// key (point) + value (tag), dim is <= the number of coordinates in each point, etc
	size_t bounds_size;
	/// split a bounds object in two along some dimension
	///
	/// the bounds object self is split into the part before root_pt (stored in o1) and the part after root_pt
	/// (stored in o2), in the dimenstion depth%ft->dim.  depth is stored in ft->super.base.data
	void (*split)(const cr8r_kd_ft *ft, const void *self, const void *root_pt, void *o1, void *o2);
	/// update a bounds object to include some point
	///
	/// self is a bounds object.  If pt is already in self, nothing is changed.  Otherwise, self is extended to
	/// include pt
	void (*update)(const cr8r_kd_ft *ft, void *self, const void *pt);
	/// find the minimum squared distance from a bounds object to a point
	double (*min_sqdist)(const cr8r_kd_ft *ft, const void *self, const void *pt);
	/// find the squared distance between two points
	double (*sqdist)(const cr8r_kd_ft *ft, const void *a, const void *b);
};

/// Concrete bounds object for spherical and cuboid kd trees with 3 i64 coords
///
/// Bounds objects describe the range of points contained in a kd tree or subtree.
/// A bounds object contains a bl (bottom left) point which is the minimum of all
/// points in each dimension (hence bottom left in 2d), and an analagous tr (top right)
/// point.  HOWEVER, bounds objects are generally passed as void pointers.
/// { @link cr8r_kdwin_init_s2i64 }, { @link cr8r_kdwin_init_c3i64 }, and
/// { @link cr8r_kdwin_bounding_i64x3 } deal with cr8r_kdwin_s2i64's directly,
/// so these functions will be replaced in a later revision of this api.
typedef struct{
	/// The minimum point across all dimensions ("bottom left")
	///
	/// Obviously this is not generally a point in the kd tree, but rather
	/// a corner of the aabb (axis aligned bounding box) in the case of a cuboid
	/// kd tree or an analagous slice of a sphere in the case of a sphere kd tree
	int64_t bl[3];
	/// The maximum point across all dimensions ("top right")
	int64_t tr[3];
} cr8r_kdwin_s2i64;

/// Type of a visitor callback for a kd tree traversal
///
/// f(cr8r_kd_ft *ft, const void *win, void *ent, void *data)
/// where the arguments are the kd tree function table, bounds of the subtree rooted at ent,
/// current node, and reentrant data respectively
typedef cr8r_walk_decision (*cr8r_kdvisitor)(cr8r_kd_ft *ft, const void *win, void *ent, void *data);

/// Initialize a i64x3 bounds object with a given "bottom left" and "top right" point
bool cr8r_kdwin_init_s2i64(cr8r_kdwin_s2i64 *self, const int64_t bl[3], const int64_t tr[3]);

/// Initialize a i64x3 bounds object with a given "bottom left" and "top right" point
bool cr8r_kdwin_init_c3i64(cr8r_kdwin_s2i64 *self, const int64_t bl[3], const int64_t tr[3]);

/// Initialize a i64x3 bounds object with the "bottom left" and "top right" points bounding a list of points
bool cr8r_kdwin_bounding_i64x3(cr8r_kdwin_s2i64 *self, const cr8r_vec *ents, const cr8r_kd_ft *ft);

/// Rearrange a vector of points into a kd tree
///
/// The middle point in the array will be the median in the first dimension, with all points to the left
/// being to the "left" in the first dimension and all points to the right being to the "right" in the
/// first dimension.  Then within the left/right subarray (subtree), the middle point is the median in the
/// second dimension and so on.
/// @param [in] _ft: (uint64_t)_ft->super.base.data is interpreted as the depth of the current subarray, and
/// so should be zero when calling this function generally on the whole vector
/// @param [in] a, b: the start (inclusive) and end (exclusive) indices of the subarray to process.
/// Should be 0, self->len to treeify the entire vector.
bool cr8r_kd_ify(cr8r_vec *self, cr8r_kd_ft *_ft, uint64_t a, uint64_t b);

/// Perform a preorder traversal of the points in a kd tree and call a visitor callback on each
///
/// @param [in] ft: (uint64_t)_ft->super.base.data is interpreted as the depth of the current subarray, and
/// so should be zero when calling this function generally on the whole vector
/// @param [in] bounds: the bounds of the tree
/// @param [in] visitor: function to call at each entry in the tree
/// @param [in, out] data: passed to visitor to facillitate input/output
void cr8r_kd_walk(cr8r_vec *self, const cr8r_kd_ft *ft, const void *bounds, cr8r_kdvisitor visitor, void *data);

/// Find the k closest points to a given point
///
/// @param [in] ft: (uint64_t)_ft->super.base.data is interpreted as the depth of the current subarray, and
/// so should be zero when calling this function generally on the whole vector
/// @param [in] bounds: the bounds of the tree
/// @param [in] pt: point to find k closest points to.  Note that if pt is in the tree, it will be returned.
/// If there is a tie for the kth closest point, the list of the k closest points may contain any of the tied points,
/// but will always contain exactly k unless there are fewer than k entries in the tree.
/// @param [in] k: the number of points to find
/// @param [out] out: vector where the k closest points will be stored, in no specified order (the current
/// implementation unsurprisingly places them in a minmax heap order)
void cr8r_kd_k_closest(cr8r_vec *self, cr8r_kd_ft *ft, const void *bounds, const void *pt, uint64_t k, cr8r_vec *out);

/// Find the k closest points to a given point
///
/// This function is designed to enable testing { @link cr8r_kd_k_closest }.  That function prunes its search area if the max
/// distance of any of the k closest points so far is less than the min distance of the bounding box of the subtree rooted ata
/// a node, then that node and its subtree can be skipped.  This function does not do that and just compares all points.
void cr8r_kd_k_closest_naive(cr8r_vec *self, cr8r_kd_ft *ft, const void *bounds, const void *pt, uint64_t k, cr8r_vec *out);

/// kdft implementation for spherical kd trees in 3 dimensions with i64 coordinates
extern cr8r_kd_ft cr8r_kdft_s2i64;
/// kdft implementation for cuboid kd trees in 3 dimensions with i64 coordintates
extern cr8r_kd_ft cr8r_kdft_c3i64;

