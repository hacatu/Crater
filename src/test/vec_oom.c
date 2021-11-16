#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <crater/vec.h>

inline static void default_copy(cr8r_base_ft *ft, void *dest, const void *src){
	memcpy(dest, src, ft->size);
}

int main(){
	fprintf(stderr, "\e[1;34mTesting vector out of memory conditions and edge cases\e[0m\n");
	uint64_t tested = 0, passed = 0;
	cr8r_vec vec;
	cr8r_vec_ft ft = cr8r_vecft_u64;
	ft.resize = cr8r_default_resize_pass;
	int status = cr8r_vec_init(&vec, &ft, 8);
	++tested;
	if(!status){
		fprintf(stderr, "\e[1;32mcr8r_vec_init reports oom correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_init fails to report oom!\e[0m\n");
	}

	cr8r_vec fake_vec = {.len = 8, .cap = 8};
	status = cr8r_vec_copy(&vec, &fake_vec, &ft);
	++tested;
	if(!status){
		fprintf(stderr, "\e[1;32mcr8r_vec_copy reports oom correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_copy fails to report oom!\e[0m\n");
	}

	ft.copy = default_copy;
	ft.resize = cr8r_default_resize;
	vec = (cr8r_vec){};
	status = cr8r_vec_resize(&vec, &ft, 8);
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_resize succeeded!\e[0m\n");
		++passed;
	}

	for(uint64_t i = 0; status && i < vec.cap; ++i){
		status = cr8r_vec_pushr(&vec, &ft, &i);
	}
	if(!status){
		fprintf(stderr, "\e[1;31mERROR: failed to initialize u64 vec!\e[0m\n");
		exit(EXIT_FAILURE);
	}

	status = cr8r_vec_copy(&fake_vec, &vec, &ft);
	status = status && fake_vec.len == vec.len;
	for(uint64_t i = 0; status && i < fake_vec.len; ++i){
		status = *(uint64_t*)cr8r_vec_get(&fake_vec, &ft, i) == i;
	}
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_copy/augment calls nontrivial ft->copy correctly\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_copy/augment does not call nontrivial ft->copy correctly\e[0m\n");
	}
	cr8r_vec_delete(&fake_vec, &ft);

	status = cr8r_vec_sub(&fake_vec, &vec, &ft, 0, vec.len);
	status = status && fake_vec.len == vec.len;
	for(uint64_t i = 0; status && i < fake_vec.len; ++i){
		status = *(uint64_t*)cr8r_vec_get(&fake_vec, &ft, i) == i;
	}
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_sub calls nontrivial ft->copy correctly\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_sub does not call nontrivial ft->copy correctly\e[0m\n");
	}
	cr8r_vec_delete(&fake_vec, &ft);

	status = cr8r_vec_sub(&fake_vec, &vec, &ft, 0, vec.len + 1);
	++tested;
	if(!status){
		fprintf(stderr, "\e[1;32mcr8r_vec_sub reports b > len correctly\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_sub does not report b > len correctly\e[0m\n");
	}

	status = cr8r_vec_sub(&fake_vec, &vec, &ft, 1, 0);
	++tested;
	if(!status){
		fprintf(stderr, "\e[1;32mcr8r_vec_sub reports b < a correctly\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_sub does not report b < a correctly\e[0m\n");
	}

	ft.resize = cr8r_default_resize_pass;
	status = cr8r_vec_sub(&fake_vec, &vec, &ft, 0, vec.len);
	++tested;
	if(!status){
		fprintf(stderr, "\e[1;32mcr8r_vec_sub reports oom correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_sub does not report oom correctly!\e[0m\n");
	}

	status = cr8r_vec_resize(&vec, &ft, 0);
	++tested;
	if(!status){
		fprintf(stderr, "\e[1;32mcr8r_vec_resize reports overtrimming correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_resize does not report overtrimming correctly!\e[0m\n");
	}

	status = cr8r_vec_resize(&vec, &ft, vec.len + 1);
	++tested;
	if(!status){
		fprintf(stderr, "\e[1;32mcr8r_vec_resize reports oom correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_resize does not report oom correctly!\e[0m\n");
	}

	status = cr8r_vec_trim(&vec, &ft);
	++tested;
	if(!status){
		fprintf(stderr, "\e[1;32mcr8r_vec_trim reports shrink fail correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_trim does not report shrink fail correctly!\e[0m\n");
	}

	ft.resize = cr8r_default_resize;
	cr8r_vec_delete(&vec, &ft);
	ft = cr8r_vecft_cstr;
	status = cr8r_vec_init(&vec, &ft, 8);
	for(uint64_t i = 0; status && i < vec.cap; ++i){
		char *cstr = NULL;
		asprintf(&cstr, "a%"PRIu64, i + 666);
		status = !!cstr && cr8r_vec_pushr(&vec, &ft, &cstr);
	}
	if(!status){
		fprintf(stderr, "\e[1;31mERROR: failed to initialize cstr vec!\e[0m\n");
		exit(EXIT_FAILURE);
	}
	cr8r_vec_clear(&vec, &ft);
	++tested;
	if(!vec.len){
		fprintf(stderr, "\e[1;32mcr8r_vec_clear calls nontrivial ft->del correctly\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_clear does not call nontrivial ft->del correctly\e[0m\n");
	}
	cr8r_vec_delete(&vec, &ft);

	ft = cr8r_vecft_u64;
	ft.resize = cr8r_default_resize_pass;
	fake_vec = (cr8r_vec){.len = 8, .cap = 8};
	status = cr8r_vec_combine(&vec, &fake_vec, &fake_vec, &ft);
	++tested;
	if(!status){
		fprintf(stderr, "\e[1;32mcr8r_vec_combine reports oom correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_combine does not report oom correctly!\e[0m\n");
	}
	
	ft.resize = cr8r_default_resize;
	ft.copy = default_copy;
	status = cr8r_vec_init(&fake_vec, &ft, 8);
	for(uint64_t i = 0; status && i < fake_vec.cap; ++i){
		status = cr8r_vec_pushr(&fake_vec, &ft, &i);
	}
	if(!status){
		fprintf(stderr, "\e[1;31mERROR: failed to initialize u64 vec!\e[0m\n");
		exit(EXIT_FAILURE);
	}
	status = cr8r_vec_combine(&vec, &fake_vec, &fake_vec, &ft);
	status = status && vec.len == 2*fake_vec.len;
	for(uint64_t i = 0; status && i < vec.len; ++i){
		status = *(uint64_t*)cr8r_vec_get(&vec, &ft, i) == i%fake_vec.len;
	}
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_combine calls nontrivial ft->copy correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_combine does not call nontrivial ft->copy correctly!\e[0m\n");
	}
	cr8r_vec_delete(&fake_vec, &ft);

	status = !cr8r_vec_get(&vec, &ft, vec.len);
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_get reports oob correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_get does not report oob correctly!\e[0m\n");
	}

	status = !cr8r_vec_getx(&vec, &ft, vec.len);
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_getx reports oob correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_getx does not report oob correctly!\e[0m\n");
	}

	cr8r_vec_delete(&vec, &ft);
	ft.resize = cr8r_default_resize_pass;
	status = !cr8r_vec_pushr(&vec, &ft, &tested);
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_pushr reports oom correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_pushr does not report oom correctly!\e[0m\n");
	}

	status = !cr8r_vec_pushl(&vec, &ft, &tested);
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_pushl reports oom correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_pushl does not report oom correctly!\e[0m\n");
	}

	uint64_t dummy_u64;
	status = !cr8r_vec_popr(&vec, &ft, &dummy_u64);
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_popr reports oob correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_popr does not report oob correctly!\e[0m\n");
	}

	status = !cr8r_vec_popl(&vec, &ft, &dummy_u64);
	++tested;
	if(status){
		fprintf(stderr, "\e[1;32mcr8r_vec_popl reports oob correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_popl does not report oob correctly!\e[0m\n");
	}

	ft.resize = cr8r_default_resize;
	if(!cr8r_vec_pushr(&vec, &ft, &tested)){
		fprintf(stderr, "\e[1;31mERROR: failed to initialize u64 vec!\e[0m\n");
		exit(EXIT_FAILURE);
	}
	ft.resize = cr8r_default_resize_pass;
	status = cr8r_vec_augment(&fake_vec, &vec, &ft);
	++tested;
	if(!status){
		fprintf(stderr, "\e[1;32mcr8r_vec_augment reports oom correctly!\e[0m\n");
		++passed;
	}else{
		fprintf(stderr, "\e[1;31mcr8r_vec_augment does not report oom correctly!\e[0m\n");
	}
	ft.resize = cr8r_default_resize;
	cr8r_vec_delete(&vec, &ft);

	if(passed == tested){
		fprintf(stderr, "\e[1;32mSuccess: passed %"PRIu64"/%"PRIu64" tests\e[0m\n", passed, tested);
	}else{
		fprintf(stderr, "\e[1;31mFailed: passed %"PRIu64"/%"PRIu64" tests\e[0m\n", passed, tested);
	}
}

