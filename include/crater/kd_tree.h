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

typedef struct{
	int64_t coords[3];
	char data[];
} cr8r_kd_i64x3_pt;

typedef struct{
	cr8r_vec points;
	int64_t radius;
} cr8r_kd_s2i64;

void cr8r_kd_s2i64_ify(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft);

cr8r_kd_i64x3_pt *cr8r_kd_s2i64_nearest(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, const int64_t point[3]);

void cr8r_kd_s2i64_forall_quad(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, const int64_t min_bound[3], const int64_t max_bound[3], void *data, void (*f)(void *data, const void *point));

void cr8r_kd_s2i64_forall_ball(cr8r_kd_s2i64 *self, cr8r_vec_ft *ft, const int64_t point[3], int64_t sqdist, void *data, void (*f)(void *data, const void *point));

