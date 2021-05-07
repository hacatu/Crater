// This test case is based on Project Euler problem 9
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <crater/vec.h>
#include <crater/hash.h>

typedef uint64_t u64_2[2];
typedef uint64_t u64_3[3];

cr8r_vec_ft vecft_u64_2, vecft_u64_3;

int fillFareyEven(cr8r_vec *self, uint64_t n){
	uint64_t a = 0, b = 1, c = 1, d = n;
    while(c <= n){
		uint64_t k = (n + b)/d;
		uint64_t t = c;
		c = k*c - a;
		a = t;
		t = d;
		d = k*d - b;
		b = t;
		if((a&1)&&(b&1)){
			continue;
		}
		u64_2 mn = {b, a};
		fprintf(stderr, "%"PRIu64"/%"PRIu64", ", b, a);
		if(!cr8r_vec_pushr(self, &vecft_u64_2, &mn)){
			return 0;
		}
	}
	fprintf(stderr, "\n");
	return 1;
}

bool m_divides_500(const void *_mn){
	const uint64_t *mn = *(const u64_2*)_mn;
	return 500%mn[0] == 0 && mn[1] <= 500/mn[0] - mn[0];
}

void mn2abc(void *_abc, const void *_mn){
	uint64_t *abc = *(u64_3*)_abc;
	const uint64_t *mn = *(const u64_2*)_mn;
	abc[0] = (mn[0] - mn[1])*(mn[0] + mn[1]);
	abc[1] = 2*mn[0]*mn[1];
	abc[2] = mn[0]*mn[0] + mn[1]*mn[1];
}

bool perim_divides_1000(const void *_abc){
	const uint64_t *abc = *(const u64_3*)_abc;
	uint64_t p = abc[0] + abc[1] + abc[2];
	return 1000%p == 0;
}

int main(){
	fprintf(stderr, "\e[1;34mSearching for integer sided right triangles with perimeter 1000...\e[0m\n");
	// a = k*(m**2 - n**2), b = k*(2*m*n), c = k*(m**2 + n**2)
	// a + b + c = 2*k*m*(m + n) = 1000
	// thus m*(m + n) <= 500, so if n == 1, m*(m + 1) <= 500 -> m <= 21
	// with m > n coprime and not both odd
	// if we want a < b, we want m**2 - n**2 < 2*m*n
	cr8r_vec vec_mns, vec_abcs;
	// we provide the required function so ft_init returns 1
	cr8r_vec_ft_init(&vecft_u64_2, NULL, sizeof(u64_2),
		cr8r_default_new_size,
		cr8r_default_resize, NULL, NULL,
		cr8r_default_cmp,
		cr8r_default_swap
	);
	vecft_u64_3 = vecft_u64_2;
	vecft_u64_3.base.size = sizeof(u64_3);
	if(!cr8r_vec_init(&vec_mns, &vecft_u64_2, 21)){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate vector!\e[0m\n");
		exit(1);
	}
	if(!cr8r_vec_init(&vec_abcs, &vecft_u64_3, 21)){
		cr8r_vec_delete(&vec_mns, &vecft_u64_2);
		fprintf(stderr, "\e[1;31mERROR: Could not allocate vector!\e[0m\n");
		exit(1);
	}
	fprintf(stderr, "\e[1;34mGenerating Farey sequence terms <= 21 with even components...\e[0m\n");
	if(!fillFareyEven(&vec_mns, 21)){
		cr8r_vec_delete(&vec_mns, &vecft_u64_2);
		cr8r_vec_delete(&vec_abcs, &vecft_u64_3);
		fprintf(stderr, "\e[1;31mERROR: Could not expand vector to contain Farey sequence!\e[0m\n");
		exit(1);
	}
	fprintf(stderr, "\e[1;34mFound %"PRIu64" terms.\e[0m\n", vec_mns.len);
	// the efficient solution is to filter by whether or not m*(m + n) divides 500 as we generate the Farey sequence,
	// but by storing candidate solutions and using a weaker filter we can get some mileage out of the vector class.
	// default_resize calls realloc which probably can't fail to shrink so we ignore the result
	cr8r_vec_filter(&vec_mns, &vecft_u64_2, m_divides_500);
	fprintf(stderr, "\e[1;34m%"PRIu64" lead to candidate triples\e[0m\n", vec_mns.len);
	if(!cr8r_vec_map(&vec_abcs, &vec_mns, &vecft_u64_2, &vecft_u64_3, mn2abc)){
		cr8r_vec_delete(&vec_mns, &vecft_u64_2);
		cr8r_vec_delete(&vec_abcs, &vecft_u64_3);
		fprintf(stderr, "\e[1;31mERROR: Could not expand map output vector!\e[0m\n");
		exit(1);
	}
	cr8r_vec_delete(&vec_mns, &vecft_u64_2);
	cr8r_vec_filter(&vec_abcs, &vecft_u64_3, perim_divides_1000);
	uint64_t *abc = cr8r_vec_getx(&vec_abcs, &vecft_u64_3, -1);
	uint64_t k = 1000/(abc[0] + abc[1] + abc[2]);
	fprintf(stderr,
		"\e[1;32m%"PRIu64", %"PRIu64", %"PRIu64" is the unique Pythagorean triple with perimeter 1000.\n"
		"The product of the elements is %"PRIu64"\e[0m\n", abc[0]*k, abc[1]*k, abc[2]*k, abc[0]*k*abc[1]*k*abc[2]*k);
	cr8r_vec_delete(&vec_abcs, &vecft_u64_3);
}

