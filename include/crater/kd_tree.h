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

/// A point and associated data in 3D space
///
/// Coordinates are int64_t's and the associated
/// data can be anything (it's stored in a flexible length array member)
typedef struct{
	int64_t coords[3];
	char data[];
} cr8r_kd_i64x3_pt;

/// A KD tree storing points on a sphere in 3D
///
/// Note that this stored as an implicit KD tree and so does not provide
/// fast insertion/removal, only fast building and retrieval.  This
/// is not suitable for use cases where a lot of insertion/removal is required.
///
/// Also, the reason that separate data structures are used for different kinds
/// of KD trees (points on a sphere in 3D, points in a rectangle in 2D, points
/// in a cuboid in 3D, etc), is because using a generic interface would be
/// very cumbersome since a lot of comparison functions would be needed.
/// This is a serious shortcoming, and eventually the library may provide
/// spherical/cylindrical KD trees, rectangular KD trees, and generic KD trees.
/// After all, the entire point of having the { @link cr8r_base_ft } pointer argument
/// to the comparison function is so that one comparison function can be used for
/// multiple coordinates and things like that.  However, that was originally
/// intended to allow users to overload functions.  Using it here differs from that
/// intent in two ways: we would have KD trees modifying their own ft->data, and we
/// would be preventing users from attaching their own data to ft->data.  However,
/// the existing KD tree implementation already modifies ft->cmp, and some functions
/// for other data structures modify ft->copy or ft->free/ft->del (this is always done
/// in a copy), and we already provide ft->alloc/ft->free for avl trees and linked
/// lists that wrap slab allocators, and these wrappers already restrict how the user
/// may define ft->data (requiring a pointer to a slab allocator as the first thing in
/// ft->data).
///
/// These are { @link cr8r_kd_i64x3_pt }'s so they have three coordinates,
/// but we can sort them in only two dimensions since they are constrained
/// to the surface of a sphere.  We use the ratio of x to y and the sign of
/// x and y as the first dimension and z as the second dimension.
typedef struct{
	/// The points and associated data in the tree.  The tree is stored
	/// implicitly in this vector, just like a standard implicit binary heap.
	/// The downside of this is insertion/removal cannot be implemented
	/// efficiently, since these require a more complicated structure like a kdb tree.
	cr8r_vec points;
	/// The radius of the sphere on which the points lie.  Note that x**2 + y**2 + z**2 = r**2
	/// so this can be computed from any point in the tree.  However, it is
	/// needed in just about every tree operation (and can't be computed from an empty tree)
	/// so it is stored separately.  We can also obviously get z from x, y, r, and sgn(z),
	/// but again this would be inefficient.
	int64_t radius;
} cr8r_kd_s2i64;

typedef struct{
	int64_t a[3], b[3];
} cr8r_kd_s2i64_quad;

typedef struct{
	int64_t p[3];
	int64_t sqdist;
} cr8r_kd_s2i64_ball;

typedef struct{
	int64_t a[3], b[3];
} cr8r_kd_c3i64_quad;

typedef struct{
	int64_t p[3];
	int64_t sqdist;
} cr8r_kd_c3i64_ball;

typedef struct{
	cr8r_vec points;
	cr8r_kd_c3i64_quad bounds;
} cr8r_kd_c3i64;

int64_t cr8r_kd_i64x3_sqdist(const int64_t a[static 3], const int64_t b[static 3]);

/// Organize the points in a spherical KD tree into the proper structure
///
/// This should be called after using { @link cr8r_vec_pushr } or similar methods
/// to populate the underlying vector self->points with the points.
/// @param [in,out] self: the KD tree to organize.  points and radius should already be set.
/// @param [in] ft: function table.  ft->cmp is set internally (in a copy) to a buildin comparator for the x/y
/// dimension or the z dimension, so the ft->cmp of the function table does not matter.
/// The size should be set to offsetof(cr8r_kd_i64x3_pt, data) + sizeof(the_data).  The copy and swap functions
/// should be set to cr8r_default_copy and swap.
void cr8r_kd_s2i64_ify(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft);

/// Find the point in a cubic KD tree nearest to a given point
///
/// WARNING: Currently there is a branch cut on the positive x half of the xz plane so queries
/// with a small y component can return the wrong result if the points are not symmetric under reflection
/// across the xz plane
/// @param [in] point: the query point, passed as an int64_t[3] rather than a cr8r_kd_i64x3_pt
/// because only the location of the query point and not any associated data is relevant
/// @return a pointer to a { @link cr8r_kd_i64x3_pt } representing the nearest point and associated data,
/// or NULL if the tree is empty.  Note that the size of the associated data is defined in ft.
cr8r_kd_i64x3_pt *cr8r_kd_s2i64_nearest(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, const int64_t point[3]);

/// Test if a "rectangular" region on the surface of a spherical KD tree intersects with a given cuboid
///
/// @param [in] r2: the radius of the spherical KD tree on which the rectangle region lives.  Note that the center of said sphere is always the origin
/// @param [in] query: the cuboid.  NOTE: the coordinates of query are coordinates in space and generally should not be on the surface of the sphere
/// @param [in] window: the "rectangular" region on the surface of the sphere.  The coordinates of window SHOULD be points on the surface of the sphere
/// @return 2 if window is entirely contained in query, 1 if window intersects query but is not entirely contained, or 0 if window and query are disjoint.
int cr8r_kd_s2i64_intersect_quad_quad(int64_t r2, cr8r_kd_s2i64_quad query, cr8r_kd_s2i64_quad window);

/// Call a function on all points in a spherical KD tree within some "rectangular" region
///
/// The "rectangular" region is the region between two given points in terms of azimuthal angle
/// and polar angle (though the checks are actually implemented in terms of sign-aware x to y ratio and
/// z coordinate).  The region includes its boundaries (ie is a closed region).
/// WARNING: If the branch cut touches the right edge or interior of the region, this function probably won't
/// work
/// @param [in] min_bound: the "top left" point of the query region, inclusive.
/// @param [in] max_bount: the "bottom right" point of the query region, INCLUSIVE
/// @param [in, out] data: scratch data for the callback function.  Any input, state, or output, should be contained here.
/// @param [in] f: the callback to call on all points in the region
void cr8r_kd_s2i64_forall_quad(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, cr8r_kd_s2i64_quad query, void *data, void (*f)(void *data, const void *point));

/// Test if a "rectangular" region on the surface of a spherical KD tree intersects with a given ball
///
/// @param [in] r2: the radius of the spherical KD tree on which the rectangle region lives.  Note that the center of said sphere is always the origin
/// @param [in] query: the ball region, whose center should generally not be the origin since that case can be handled more easily by just comparing radii
/// @param [in] window: the "rectangular" region on the surface of the sphere
/// @return 2 if window is entirely contained in query, 1 if window intersects query but is not entirely contained, or 0 if window and query are disjoint.
int cr8r_kd_s2i64_intersect_ball_quad(int64_t r2, cr8r_kd_s2i64_ball query, cr8r_kd_s2i64_quad window);

/// Call a function on all points in a spherical KD tree within some distance of a given point
///
/// Very similar to { @link cr8r_kd_s2i64_forall_quad } but with a "circular" query region rather than a
/// "rectangular" one.  This has the same asymptotic complexity but is a more complicated search in practice
/// because the kd tree splits up the points in an inherently rectangular manner.
/// This function exemplifies the main reason why we store the points as x, y, z coordinates rather than azimuthal and
/// polar angles: for angles the distance formula is D**2 = 2*r**2*(1 - sin(theta)*sin(theta')*cos(phi-phi') - cos(phi)*cos(phi')),
/// while for rectangular coordinates it is simply D**2 = dx**2 + dy**2 = dz**2.
/// In general, formulae using rectangular coordinates are simpler and converting from angles to rectangular coordinates requires
/// trig.  On the other hand, recovering the angles from the rectangular coordinates requires inverse trig, but we can
/// often get around this somehow (using ratios, areas, cross/dot product, etc).
/// The square distance is used to avoid a square root and to keep numbers in the integers.
/// This is the squared euclidean distance between the points in R^3, that is, dx**2 + dy**2 + dz**2.
void cr8r_kd_s2i64_forall_ball(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, cr8r_kd_s2i64_ball query, void *data, void (*f)(void *data, const void *point));

/// Call a function on all points in a spherical KD tree
///
/// Very similar to { @link cr8r_kd_s2i64_forall_quad } except this function visits ALL points unconditionally.
void cr8r_kd_s2i64_forall(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, void *data, void (*f)(void *data, const void *point));

/// Organize the points in a cuboid KD tree into the proper structure
///
/// This should be called after using { @link cr8r_vec_pushr } or similar methods
/// to populate the underlying vector self->points with the points.
/// @param [in,out] self: the KD tree to organize.  points and bounds should already be set.
/// @param [in] ft: function table.  ft->cmp is set internally (in a copy) to a buildin comparator for the x, y,
/// or z dimension, so the ft->cmp of the function table does not matter.
/// The size should be set to offsetof(cr8r_kd_i64x3_pt, data) + sizeof(the_data).  The copy and swap functions
/// should be set to cr8r_default_copy and swap.
void cr8r_kd_c3i64_ify(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft);

/// Find the point in a cuboid KD tree nearest to a given point
///
/// @param [in] point: the query point, passed as an int64_t[3] rather than a cr8r_kd_i64x3_pt
/// because only the location of the query point and not any associated data is relevant
/// @return a pointer to a { @link cr8r_kd_i64x3_pt } representing the nearest point and associated data,
/// or NULL if the tree is empty.  Note that the size of the associated data is defined in ft.
cr8r_kd_i64x3_pt *cr8r_kd_c3i64_nearest(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft, const int64_t point[3]);

/// Call a function on all points in a cuboid KD tree within some cuboid region
///
/// The region includes its boundaries (ie is a closed region).
/// @param [in] min_bound: the "top left back" point of the query region, inclusive.
/// @param [in] max_bount: the "bottom right front" point of the query region, INCLUSIVE
/// @param [in, out] data: scratch data for the callback function.  Any input, state, or output, should be contained here.
/// @param [in] f: the callback to call on all points in the region
void cr8r_kd_c3i64_forall_quad(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft, cr8r_kd_c3i64_quad query, void *data, void (*f)(void *data, const void *point));

/// Test if a cuboid region in a cuboid KD tree intersects with a given ball
///
/// @param [in] query: the ball region (in space)
/// @param [in] window: the window region (in the KD tree)
/// @return 2 if window is entirely contained in query, 1 if window intersects with query but is not entirely contained, or 0 if window and query do not intersect
int cr8r_kd_c3i64_intersect_ball_quad(cr8r_kd_c3i64_ball query, cr8r_kd_c3i64_quad window);

/// Call a function on all points in a cuboid KD tree within some distance of a given point
///
/// Very similar to { @link cr8r_kd_c3i64_forall_quad } but with a spherical query region rather than a
/// cuboid one.
/// The square distance is used to avoid a square root and to keep numbers in the integers.
/// This is the squared euclidean distance between the points in R^3, that is, dx**2 + dy**2 + dz**2.
void cr8r_kd_c3i64_forall_ball(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft, cr8r_kd_c3i64_ball query, void *data, void (*f)(void *data, const void *point));

/// Call a function on all points in a cuboid KD tree
///
/// Very similar to { @link cr8r_kd_c3i64_forall_quad } except this function visits ALL points unconditionally.
void cr8r_kd_c3i64_forall(cr8r_kd_c3i64 *self, cr8r_vec_ft *ft, void *data, void (*f)(void *data, const void *point));

