# Crater Container Library


[![Documentation](https://img.shields.io/badge/-documentation-gray)](https://hacatu.github.io/Crater/docs)
[![Coverage](https://hacatu.github.io/Crater/build/debug/coverage.svg)](https://hacatu.github.io/Crater/build/debug/cov)

Crater provides simple, fast generic collections in plain C.
All containers can be parameterized by specifying callback functions in a "function table".

Also includes a slab allocator to support allocating nodes in linked structures more efficiently than malloc.

AVL trees, hash tables, vectors, and circular linked lists are currently provided, and both vectors and AVL trees include
heap functions.  Pairing heaps, k-d trees, and possibly red black trees and double linked lists are planned.

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

`-fno-strict-aliasing` is important to keep because the elements in most containers are stored in flexible length
`char` arrays, and strict aliasing has to be disabled to allow a two way conversion between `char*` and `T*`.

## Linking
Once the library has been built, it can be linked with C programs by adding the flags `-Lbuild/debug/lib -lcrater`
or `-L$(BUILD_ROOT)/lib -lcrater` to use a different variant build.  If you're feeling dangerous, you can install
it to your system libraries by doing `sudo cp build/debug/lib/libcrater.a /usr/lib/` or
`sudo cp $(BUILD_ROOT)/lib/libcrater.a /usr/lib`.  Then the `-L` flag can be omitted and the library can be
linked with simply `-lcrater`.

## Examples

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
		if(!cr8r_vec_resize(&rseq, &ft, 7)){
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

