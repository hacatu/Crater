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

This library uses variant makefiles for each build type.  To build the debug variant, simply run `make`
in the project root directory.  The resulting files will be in `build/debug/lib` and `build/debug/bin`.

To build a different variant, change the `$(BUILD_ROOT)` variable from `build/debug` to `build/release`
or some other value, for instance `make BUILD_ROOT="build/release"`.  You can check what variants exist
by looking at the subdirectories of `build`, and you can create your own by copying and modifying `build/debug`.

The files in a variant build directory are arranged as follows: `Makefile` is the makefile, variants
probably won't have to modify this much aside from removing coverage information; `cflags.txt`,
`ldflags.txt`, and `makedeps_cflags.txt` contain configuration flags that should be passed to the compiler
and linker.  Flag files are used to consolidate compiler flags, reduce how often the makefile needs to be
tweaked, and clean up build logs.

Based on the source files in `include` and `src`, together with the configuration in a variant build directory,
output files are produced in the following subdirectories of `$(BUILD_ROOT)`: `bin` for executables, `bin/test`
for test executables, `lib` for static and dynamic libraries, `obj` for object files and dependency files,
`notes` for coverage information, `log` for test logs, and `cov` for human readable coverage reports.

Executables, static libraries, and test executables are automatically created based on the contents of `src`:
every C file or directory in `src/bin` is turned into its own executable in `$(BUILD_ROOT)/bin`, every C file
or directory in `src/lib` is turned into its own static library in `$(BUILD_ROOT)/lib`, and every C file or
directory in `src/test` is turned into its own test executable in `$(BUILD_ROOT)/bin/test`.

To run tests, simply run `make test` (or `make BUILD_ROOT=build/release test`).  For build variants where
coverage testing should be done, `make coverage` is better since both will re-run all tests every time.

`make clean` should remove all output files in all variant build directories, as well as all generated
documentation.

Finally, `make docs` generates the documentation in the `docs` directory.  Like `make clean`, this is not
tied to a build variant and even if you specify one the same thing will happen.

`make debug_makefile` simply exists to facillitate printing make variables, don't worry about it.

Only Linux is properly supported.  To build, only `gcc`, `ar`, and `make` are strictly required, but `lcov`
and `doxygen` are required for coverage and documentation, and `gold` is specified as the linker by default.
If you do not have `gold`, you can switch the line `-fuse-ld=gold` to `-fuse-ld=lld` or remove it in
`$(BUILD_ROOT)/ldflags.txt`.

Further, some of the tests need `SDL2`.

`-fno-strict-aliasing` is important to keep because the elements in most containers are stored in flexible length
`char` arrays, and strict aliasing has to be disabled to allow a two way conversion between `char*` and `T*`.

## Linking
Once the library has been built, it can be linked with C programs by adding the flags `-Lbuild/debug/lib -lcrater`
or `-L$(BUILD_ROOT)/lib -lcrater` to use a different variant build.  If you're feeling dangerous, you can install
it to your system libraries by doing `sudo cp build/debug/lib/libcrater.a /usr/lib/` or
`sudo cp $(BUILD_ROOT)/lib/libcrater.a /usr/lib`.  Then the `-L` flag can be omitted and the library can be
linked with simply `-lcrater`.

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

