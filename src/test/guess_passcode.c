// This test case is based on Project Euler problem 79
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crater/vec.h>

bool strminlen_pred(const cr8r_vec_ft *ft, const void *_ent, void *_data){
	const char *const *ent = _ent;
	return strlen(*ent) >= (uint64_t)_data;
}

const char *givens[50] = {
	"319", "680", "180", "690", "129",
	"620", "762", "689", "762", "318",
	"368", "710", "720", "710", "629",
	"168", "160", "689", "716", "731",
	"736", "729", "316", "729", "729",
	"710", "769", "290", "719", "680",
	"318", "389", "162", "289", "162",
	"718", "729", "319", "790", "680",
	"890", "362", "319", "760", "316",
	"729", "380", "319", "728", "716"
};

int main(int argc, char **argv){
	cr8r_vec substrs;
	cr8r_vec_init(&substrs, &cr8r_vecft_cstr, 50);
	for(uint64_t i = 0; i < 50; ++i){
		char *tmp = strdup(givens[i]);
		cr8r_vec_pushr(&substrs, &cr8r_vecft_cstr, &tmp);
	}
	cr8r_vec passcode;
	cr8r_vec_init(&passcode, &cr8r_vecft_u8, 150);
	while(substrs.len){
		uint64_t first_mask = 0, second_mask = 0;
		for(uint64_t i = 0; i < substrs.len; ++i){
			const char *substr = *(const char**)cr8r_vec_getx(&substrs, &cr8r_vecft_cstr, i);
			first_mask |= 1ull << (substr[0] - '0');
			second_mask |= 1ull << (substr[1] - '0');
		}
		first_mask &= ~second_mask;
		if(!first_mask){
			fprintf(stderr, "\e[1;31mNo characters occur only in the first position; simple algorithm failed!\e[0m\n");
			cr8r_vec_delete(&passcode, &cr8r_vecft_u8);
			cr8r_vec_delete(&substrs, &cr8r_vecft_cstr);
			exit(EXIT_FAILURE);
		}
		char chr = __builtin_ctzll(first_mask) + '0';
		cr8r_vec_pushr(&passcode, &cr8r_vecft_u8, &chr);
		for(uint64_t i = 0; i < substrs.len; ++i){
			char *substr = *(char**)cr8r_vec_get(&substrs, &cr8r_vecft_cstr, i);
			if(substr[0] == chr){
				memmove(substr, substr + 1, strlen(substr));
			}
		}
		cr8r_vec_filter(&substrs, &cr8r_vecft_cstr, strminlen_pred, (void*)2);
		if(!substrs.len){
			while(second_mask){
				int d = __builtin_ctzll(second_mask);
				char chr = d + '0';
				cr8r_vec_pushr(&passcode, &cr8r_vecft_u8, &chr);
				second_mask ^= 1ull << d;
			}
		}
	}
	cr8r_vec_delete(&substrs, &cr8r_vecft_cstr);
	if(passcode.len == 8 && !strncmp((const char*)passcode.buf, "73162890", 8)){
		fprintf(stderr, "\e[1;32mFound correct passcode: \"%.*s\"\nSuccess: passed 1/1 tests\e[0m\n", (int)passcode.len, (const char*)passcode.buf);
	}else{
		fprintf(stderr, "\e[1;31mFound incorrect passcode: \"%.*s\"\nFailed: passed 0/1 tests\e[0m\n", (int)passcode.len, (const char*)passcode.buf);
	}
	cr8r_vec_delete(&passcode, &cr8r_vecft_u8);
}

