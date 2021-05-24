# Crater Container Library

[![GitHub CI](https://github.com/hacatu/Crater/actions/workflows/cov_and_docs.yml/badge.svg)](https://github.com/hacatu/Crater/actions/workflows/cov_and_docs.yml)
[![Documentation](https://img.shields.io/badge/-documentation-gray)](https://hacatu.github.io/Crater/docs)
[![Coverage](https://hacatu.github.io/Crater/coverage.svg)](https://hacatu.github.io/Crater/cov)

Crater provides simple, fast generic collections in plain C and some other general purpose utilities.
All containers can be parameterized by specifying callback functions in a "function table".

### Features
- AVL trees
	- Ordered map/dictionary like interface with `O(log(n))` worst case time for insertion, removal, and lookup
	- Amortized `O(1)` time to find next element in inorder/postorder traversal
	- Inserting an entry with the same key as an existing entry can optionally update the existing value (eg by adding them)
	- Node allocator is configurable (see slab allocator)
	- Include operations on nodes (remove and insert existing nodes rather than entries to decrease allocations)
	- Support finding lowest upper bound/highest lower bound node
	- Existing AVL trees can be reordered according to a different sorting function or as a heap
	- Can be used as ordered sets (by having the entry type only consist of information that the comparison function considers, ie just a "key" with no "value")
- Hash tables
	- Unordered map/dictionary like interface with `O(1)` average case time for insertion, removal, and lookup
	- Amortized `O(1)` time to find next element in iteration (order of iteration is unspecified)
	- Inserting an entry with the same key as an existing entry can optionally update the existing value (eg by adding them)
	- Use incremental resizing (have two tables internally while resizing and amortize moving entries over
	 insert/remove operations).  Entries are stored directly in the hash table using quadratic probing on collisions.
- Vectors
	- Self resizing array with standard constant time operations
	- Allocator and growth rate are configurable (can specify function to compute new size)
	- Include pushl/popl (linear time left access) and `O(nlog(n))` sorting (using heapsort)
	- Support using vectors as heaps (min and max with configurable comparison function)
	- Include operations for sorted vectors (find element index in sorted list, etc)
- Circularly linked lists
	- Exist
	- Not currently tested
- Slab allocator
	- Group together many fixed size allocations so that allocating nodes in linked structures can be handled more efficiently than malloc
	- Can grow internal storage (exponentially) without invalidating already allocated nodes
- Pseudorandom Number Generators
	- Linear Congruential Generator, Lagged Fibonacci Subtract with Carry, Lagged Fibonacci Multiplication, Mersenne Twister, Xoroshiro256**,
	 SplitMix64, and Linux `/dev/random`
	- Can generate random `uint32_t`s, `uint64_t`s, uniform `uint64_t`s in a range, random bytes into a buffer, and random `double`s on `[0,1)`
	 directly
	- Some prng types support jumping to facillitate setting up multiple independent streams.  All will eventually be supported.
	 See prng documentation for details
	- These are all well tested prngs with good characteristics (and some drawbacks), and we run a few tests to confirm they work correctly:
	 byte distribution, byte correlation, and ising model

- Command line argument parsing
	- Flexible system allows optional and required options
	- Options may have required, optional, or no argument
	- Options may have short and long names
	- Type generic macro can be used to easily configure options for any scalar type by only specifying destination pointer, short name,
	 long name, description, and whether or not the option is required
	- Options can specify `on_missing` behavior to generate a random value if an optional argument is missing, or do anything else in the event
	 of a missing option
	- Required arguments are characterized by having `on_missing` be a null function pointer
	- Inspired by argparse, but with support for `on_missing` and any type
	- Namely, `char`, `_Bool`, signed and unsigned versions of `char`, `short`, `int`, `long`, `long long`, and `__int128`, plus `float`,
	 `double`, and `long double` are supported by `CR8R_OPT_GENERIC_OPTIONAL` and `CR8R_OPT_GENERIC_REQUIRED`.  `_Complex *` and `_Imaginary *`
	 are not currently supported by the type generic macros, but these and any aggregate types can be implemented with a custom `on_opt` callback.
	 A custom callback can also be used to perform a narrower range check, like forcing a number to be 0-100
	- Default help option to print help and descriptions is provided, and some options for argument parsing behavior are available (whether all/only
	 one error is reported as well as basic support for positional arguments)

Pairing heaps, k-d trees, and possibly red black trees and double linked lists are planned, as well as more support for jumping in the prng part of
the library and expansion of command line argument handling.

Each container type has an associated function table type.
For example, the AVL tree ft has `alloc`, `free`, `cmp`, and `add` callbacks, to allocate nodes,
free nodes, compare elements, and combine two elements
(to parameterized behavior for inserting existing elements in order to create a counter or something more advanced)
respectively.

Some convenient default callbacks are also included.

## Building

### Overview/Linux

Project directories:

- `include/crater`: Contains header files.  Can be copied into system include directory
 without worry of clashes.
- `src/bin`: Any file `<name>.c` in this directory is compiled into a binary
 `$(BUILD_ROOT)/bin/<name>`.  Any directory `<name>` in this directory is compiled
 into a binary `$(BUILD_ROOT)/bin/<name>`.
- `src/lib`: Any file `<name>.c` in this directory is compiled into a static library
 `$(BUILD_ROOT)/lib/lib<name>.a`.  Any directory `<name>` in this directory is compiled
 into a static library `$(BUILD_ROOT)/lib/lib<name>.a`.
- `src/test`: Any file `<name>.c` in this directory is compiled into a binary
 `$(BUILD_ROOT)/bin/test/<name>`.  Any directory `<name>` in this directory is compiled
 into a binary `$(BUILD_ROOT)/bin/test/<name>`.
- `$(BUILD_ROOT)`: all output files are put in the `$(BUILD_ROOT)`
 directory, which is `build/debug` by default, but can be changed to use a different
 build root.  Each build root represents a variant build.  The build root directory
 should contain:
	- `Makefile`: Invoked by the top level makefile to build the selected variant.
	 A good way to create a new variant is to copy this file and edit it.
	- `cflags.txt`: Compilation flags for this variant build.
	- `ldflags.txt`: Linking flags for this variant build.
	- `makedeps_cflags.txt`: Flags to pass to the C compiler to emit Makefile
	 dependency information.  `-MM -MQ $@ -MG -MP $^ -MF` will be added to this.
	 This should not need to be changed for new variant builds.
- `$(BUILD_ROOT)/bin`: Output folder for compiled binaries.
- `$(BUILD_ROOT)/lib`: Output folder for compiled static librares.
- `$(BUILD_ROOT)/bin/test`: Output folder for compiled test binaries.
- `$(BUILD_ROOT)/notes`: Output folder for coverage information.  Not flat internally:
 many files will be placed in subfolders.
- `$(BUILD_ROOT)/obj`: Output folder for intermediate object files.  Not flat 
 internally, many files will be placed in subfolders.
- `$(BUILD_ROOT)/log`: Output folder for test output.
- `test`: Contains `test.py`, `tests.json`, and expected output for applicable tests.
- `resources`: Contains non-code resources for tests (and binaries and libraries) to
 read.
- `website`: Contains the manually created parts of the website.  Lcov and Doxygen
 then generate the coverage report and documentation pages.

This library uses variant makefiles for each build type.  To build the debug variant, simply run `make`
in the project root directory.  The resulting files will be in `build/debug/lib` and `build/debug/bin`.

To build a different variant, change the `$(BUILD_ROOT)` variable from `build/debug` to `build/release`
or some other value, for instance `make BUILD_ROOT="build/release"`.  You can check what variants exist
by looking at the subdirectories of `build`, and you can create your own by copying and modifying `build/debug`.

To run tests, simply run `make test` (or `make BUILD_ROOT=build/release test`).  For build variants where
coverage testing should be done, `make coverage` is better since both will re-run all tests every time.

`make clean` should remove all output files in all variant build directories, as well as all generated
documentation.

Finally, `make docs` generates the documentation in the `docs` directory.  Like `make clean`, this is not
tied to a build variant and even if you specify one the same thing will happen.

Only Linux is properly supported.  To build, only `gcc`/`clang`, `ar`, and `make` are strictly required.  `gold` is specified as the linker by default.  Tests require `python 3` and `SDL2`, coverage requires `lcov`, and documentation requires `doxygen`.
If you do not have `gold`, you can remove the line `-fuse-ld=gold` in `$(BUILD_ROOT)/ldflags.txt`.

`-fno-strict-aliasing` is important to keep because the elements in most containers are stored in flexible length
`char` arrays, and strict aliasing has to be disabled to allow a two way conversion between `char*` and `T*`.

To use clang instead of gcc, `make CC=clang LD=clang test` can be used.  You may wish to edit `build/debug/ldflags.txt`
to not use gold since it causes some link warnings in clang.

### Windows

Try installing the Windows subsystem for Linux or setting Crater up in VSCode.
VSCodium on Linux is able to build and manage Crater pretty well, using the
`clangd` and `Makefile Tools` extensions.  You will need to create a copy of
`build/debug/cflags.txt` in the project root called `compile_flags.txt` but
with the `-I../../include` line changed to `-Iinclude`, this should make `clangd`
work properly.  Simply running the make commands in VSCode's bash terminal is
recommended because the `Makefile Tools` extension does not handle ANSI escape
sequences so its output is hard to read.

## Linking
Once the library has been built, it can be linked with C programs by adding the flags `-Lbuild/debug/lib -lcrater`
or `-L$(BUILD_ROOT)/lib -lcrater` to use a different variant build.  If you're feeling dangerous, you can install
it to your system libraries by doing `sudo cp build/debug/lib/libcrater.a /usr/lib/` or
`sudo cp $(BUILD_ROOT)/lib/libcrater.a /usr/lib`.  Then the `-L` flag can be omitted and the library can be
linked with simply `-lcrater`.

## Tests
`test/test.py` uses `test/tests.json` to determine what tests to run.  This json
configuration looks like `{<name>: {<test_kind>: [<arg_list>]}}` where
- `<name>` is the name of the test executable in `$(BUILD_ROOT)/bin/test`
- `<test_kind>` specifies how to validate the results of the tests in the associated
 array, for example:
	- `no_red_tests`: tests in `test_cfg[<name>]["no_red_tests"]` are considered to
	 pass as long as they do not output red text (the ansi control sequence
	 `\\033[1;31m`)
	- `log_diff_tests`: test `i` in the list `test_cfg[<name>]["log_diff_tests"]` is
	 considered to pass if its output matches with the file `test/<name>.i.log`.
- `<arg_list>` is a list of command line arguments to pass to the test executable.
 Paths should be relative to `$(BUILD_ROOT)`.

The configuration for the tests for `<name>` is a json object whose keys are
`<test_kind>` values and whose values are lists of `<arg_lists>`.  For each
`<name>`, for each `<test_kind>`, the test executable for `<name>` is run with each
`<arg_list>` and validated according to that test kind.

Notice `{"no_red_tests": [[]]}` is the basic way to run the test executable for
`<name>` once with no arguments and check it does not output any red text.

`test/test.py` is automatically invoked by `make test` with `$(BUILD_ROOT)` as the
working directory.  If you wish to invoke it manually, make sure to switch directories
first.

## Examples

Look in src/test for more examples!

### Command Line Arguments (from src/test/ising/render.c)
```C
#include <crater/opts.h>
#include <crater/prand.h>

...

bool generate_random_seed(cr8r_opt *self){
	cr8r_prng *sys_rng = cr8r_prng_init_system();
	if(!sys_rng){
		fprintf(stderr, "\e[1;31mERROR: Could not initialize sys rng!\e[0m\n");
		return false;
	}
	cr8r_prng_get_bytes(sys_rng, sizeof(uint64_t), self->dest);
	fprintf(stderr, "\e[1;33mseed=%"PRIx64"\e[0m\n", *(uint64_t*)self->dest);
	free(sys_rng);
	return true;
}

int main(int argc, char **argv){
	uint64_t n = 400;
	double B = 10;
	uint64_t seed;

	cr8r_opt options[] = {
		CR8R_OPT_HELP(options, "Crater Ising model test/demo\n"
			"Written by hacatu\n\n"
			"Usage:\n"
			"\t`ising`: run automated tests (no gui)\n"
			"\t`ising <options>` run SDL2 demo\n\n"),
		CR8R_OPT_GENERIC_OPTIONAL(&n, "n", "side", "lattice side length (default 400, must be positive)"),
		CR8R_OPT_GENERIC_OPTIONAL(&B, "B", "beta", "inverse temperature (default 10, must be positive)"),
		{.dest = &seed, .arg_mode = CR8R_OPT_ARGMODE_REQUIRED,
			.short_name = "s", .long_name = "seed",
			.description = "seed for prng (64 bit number), if not specified, a random seed is used and logged",
			.on_opt = cr8r_opt_parse_ull, .on_missing = generate_random_seed},
		CR8R_OPT_END()
	};
	if(!cr8r_opt_parse(options, &((cr8r_opt_cfg){.stop_on_first_err=true}), argc, argv)){
		exit(1);
	}
	...
}
```

### Test All Insert/Remove Sequences for size 7 AVL Trees
```C
	#include <stdio.h>
	#include <stdlib.h>
	#include <assert.h>

	#include <crater/avl.h>
	#include <crater/sla.h>
	#include <crater/vec.h>

	cr8r_avl_ft avlft_u64sla;

	void processRemoveSequence(const cr8r_vec *rseq, const cr8r_vec_ft *rseq_ft){
		cr8r_vec *iseq = rseq_ft->base.data;
		cr8r_avl_node *r = NULL, *it;
		for(uint64_t i = 0; i < iseq->len; ++i){
			assert(cr8r_avl_insert(&r, cr8r_vec_get(iseq, rseq_ft, i), &avlft_u64sla));
		}
		it = cr8r_avl_first(r);
		for(uint64_t i = 0; it; ++i, it = cr8r_avl_next(it)){
			assert(i == *(uint64_t*)(it->data));
		}
		for(uint64_t i = 0; i < rseq->len; ++i){
			assert(cr8r_avl_remove(&r, cr8r_vec_get((cr8r_vec*)rseq, rseq_ft, i), &avlft_u64sla));
		}
		assert(!r);
	}

	void processInsertSequence(const cr8r_vec *iseq, const cr8r_vec_ft *iseq_ft){
		fprintf(stderr, "\e[1;34m(");
		for(uint64_t i = 0; i < iseq->len; ++i){
			fprintf(stderr, i == iseq->len - 1 ? "%"PRIu64")\e[0m\n" : "%"PRIu64",", *(uint64_t*)cr8r_vec_get((cr8r_vec*)iseq, iseq_ft, i));
		}
		cr8r_vec *rseq = iseq_ft->base.data;
		cr8r_vec_ft rseq_ft = *iseq_ft;
		rseq_ft.base.data = (cr8r_vec*)iseq;
		cr8r_vec_clear(rseq, &rseq_ft);
		for(uint64_t i = 0; i < 7; ++i){
			cr8r_vec_pushr(rseq, &rseq_ft, &i);
		}
		cr8r_vec_forEachPermutation(rseq, &rseq_ft, processRemoveSequence);
	}

	int main(){
		cr8r_vec iseq, rseq;
		cr8r_vec_ft ft = cr8r_vecft_u64;
		ft.base.data = &rseq;
		if(!cr8r_vec_init(&iseq, &ft, 7)){
			fprintf(stderr, "\e[1;31mERROR: Could not allocate vector.\e[0m\n");
			exit(1);
		}
		if(!cr8r_vec_init(&rseq, &ft, 7)){
			fprintf(stderr, "\e[1;31mERROR: Could not allocate vector.\e[0m\n");
			exit(1);
		}
		cr8r_sla sla;
		if(!cr8r_avl_ft_initsla(&avlft_u64sla, &sla, sizeof(uint64_t), 7, cr8r_default_cmp_u64, NULL)){
			fprintf(stderr, "\e[1;31mERROR: Could not reserve slab allocator.\e[0m\n");
			exit(1);
		}
		for(uint64_t i = 0; i < 7; ++i){
			cr8r_vec_pushr(&iseq, &ft, &i);
		}
		cr8r_vec_forEachPermutation(&iseq, &ft, processInsertSequence);
		cr8r_sla_delete(&sla);
		cr8r_vec_delete(&iseq, &ft);
		cr8r_vec_delete(&rseq, &ft);
	}
```

