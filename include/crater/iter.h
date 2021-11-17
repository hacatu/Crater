#pragma once

/// @file
/// @author hacatu
/// @version 0.3.0
/// Detailed, structured interfaces for iterating over arbitrary data structures in a generic
/// way.  This file is highly experimental!
///
/// This Source Code Form is subject to the terms of the Mozilla Public
/// License, v. 2.0. If a copy of the MPL was not distributed with this
/// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

typedef struct HAC_ITER_ft HAC_ITER_ft;
struct HAC_ITER_ft{
	bool (*beginl)(void *self, HAC_ITER_ft*, void *ctr);
	bool (*endl)(void *self, HAC_ITER_ft*, void *ctr);
	bool (*beginr)(void *self, HAC_ITER_ft*, void *ctr);
	bool (*endr)(void *self, HAC_ITER_ft*, void *ctr);
	uint64_t (*position)(void *self, HAC_ITER_ft*);
	void *(*next)(void *self, HAC_ITER_ft*);
	void *(*prev)(void *self, HAC_ITER_ft*);
	void *(*step)(void *self, HAC_ITER_ft*, int64_t n);
	void *(*get)(void *self, HAC_ITER_ft*);
	void *(*get_offset)(void *self, HAC_ITER_ft*, int64_t n);
	bool (*set)(void *self, HAC_ITER_ft*, void *val);
	bool (*set_offset)(void *self, HAC_ITER_ft*, int64_t n, void *val);
	bool (*insert)(void *self, HAC_ITER_ft, void *val);
	bool (*insert_offset)(void *self, HAC_ITER_ft, int64_t n, void *val);
	void *(*remove)(void *self, HAC_ITER_ft*);
	void *(*remove_offset)(void *self, HAC_ITER_ft*, int64_t n);
	void *(*split)(void *self, HAC_ITER_ft*);
	void *(*split_offset)(void *self, HAC_ITER_ft*, int64_t n);
	bool (*copy)(void *self, HAC_ITER_ft*, void *other);
	uint64_t traits;
	void *data;
};

typedef enum{
	HAC_ITER_INPUT = 1ull << 0,
	HAC_ITER_OUTPUT = 1ull << 1,
	HAC_ITER_INSERT = 1ull << 2,
	HAC_ITER_REMOVE = 1ull << 3,
	HAC_ITER_SPLIT = 1ull << 4,
	HAC_ITER_FORWARD = 1ull << 5,
	HAC_ITER_REVERSE = 1ull << 6,
	HAC_ITER_POSITION = 1ull << 7,
	HAC_ITER_STEP = 1ull << 8,
	HAC_ITER_OFFSET = 1ull << 9,
	HAC_ITER_COPY = 1ull << 10
} HAC_ITER_TRAITS;

