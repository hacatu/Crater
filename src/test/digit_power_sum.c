// based on project euler problem 30
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <crater/vec.h>

static uint64_t self_dps(uint64_t abcd){
	static const uint64_t d5[10] = {0, 1, 32, 243, 1024, 3125, 7776, 16807, 32768, 59049};
	uint64_t digits[6] = {[4] = 40};
	uint64_t s = 0, s5 = 0;
	for(uint64_t i = 0, x = abcd; i < 4; ++i){
		digits[3-i] = x%10;
		digits[4] -= digits[3-i];
		x /= 10;
	}
	digits[4] %= 10;
	for(uint64_t i = 0; i <= 4; ++i){
		s += digits[i];
		s5 += d5[digits[i]];
	}
	switch((s5 - s)%9){
	case 0:
		if(abcd*100 + digits[4]*10 == s5){
			digits[5] = 0; // or 1
		}else if(abcd*100 + digits[4]*10 + 8 == s5 + d5[8]){
			digits[5] = 8;
		}else if(abcd*100 + digits[4]*10 + 9 == s5 + d5[9]){
			digits[5] = 9;
		}else{
			return 0;
		}
		break;
	case 3:
		for(uint64_t i = 3; i < 9; i += 2){
			if(abcd*100 + digits[4]*10 + i == s5 + d5[i]){
				digits[5] = i;
			}
		}
		if(!digits[5]){
			return 0;
		}
		break;
	case 6:
		for(uint64_t i = 2; i < 8; i += 2){
			if(abcd*100 + digits[4]*10 + i == s5 + d5[i]){
				digits[5] = i;
			}
		}
		if(!digits[5]){
			return 0;
		}
		break;
	}
	return abcd*100 + digits[4]*10 + digits[5];
}

static bool dps_pred(const cr8r_vec_ft *ft, const void *ent, void *data){
	uint64_t abcd = *(const uint64_t*)ent;
	return !!self_dps(abcd);
}

static void nontrivial_swap(cr8r_base_ft *ft, void *a, void *b){
	cr8r_default_swap(ft, a, b);
}

static void nontrivial_del(cr8r_base_ft *ft, void *a){}

int main(){
	self_dps(41);
	cr8r_vec vec;
	cr8r_vec_ft ft = cr8r_vecft_u64;
	ft.swap = nontrivial_swap;
	ft.del = nontrivial_del;
	cr8r_vec_init(&vec, &ft, 2952);
	// we have that x >= 10**(n-1) and s5(x) <= 9**5*n
	// so we can see that x > s5(x) whenever n > 6, so n <= 6.
	// this means x <= 6*9**5 == 354294, which means in fact a <= 3 so
	// x <= 3**5 + 5*9**5 == 295488, so a <= 2 so
	// x <= 2**5 + 5*9**5 == 295277.  In fact, we can't have 5 9s if a is 2,
	// so we actually get x <= max(s5(295277), s5(295269), s5(295199), s5(294999), s5(289999), s5(199999))
	// x <= max(95852, 129063, 180305, 237252, 268996, 295246)
	// x <= 295246.  sadly if we repeat this we do not get a better upper bound.
	for(uint64_t i = 0; i <= 2952; ++i){
		cr8r_vec_pushr(&vec, &ft, &i);
	}
	cr8r_vec_filter(&vec, &ft, dps_pred, NULL);
	fprintf(stderr, "\e[1;33mNumbers equal to the sum of the 5th powers of their digits: 0, 1\e[0m");
	uint64_t s = 0;
	for(uint64_t i = 0; i < vec.len; ++i){
		uint64_t abcd = *(const uint64_t*)cr8r_vec_get(&vec, &ft, i);
		uint64_t x = self_dps(abcd);
		if(x%10 == 0){
			s += 2*x + 1;
			fprintf(stderr, "\e[1;33m, %"PRIu64", %"PRIu64"\e[0m", x, x + 1);
		}else{
			s += x;
			fprintf(stderr, "\e[1;33m, %"PRIu64"\e[0m", x);
		}
	}
	cr8r_vec_delete(&vec, &ft);
	if(s == 443839){
		fprintf(stderr, "\n\e[1;32mGot correct sum %"PRIu64"\nSuccess: passed 1/1 tests\e[0m\n", s);
	}else{
		fprintf(stderr, "\e[1;31mGot incorrect sum %"PRIu64"\nFailed: passed 0/1 tests\e[0m\n", s);
	}
}

