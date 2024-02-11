#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crater/hash.h>
#include <crater/vec.h>

static cr8r_hashtbl_ft ft = {
	.base.size = sizeof(int64_t),
	.cmp = cr8r_default_cmp_i64,
	.hash = cr8r_default_hash_u64,
	.load_factor = 0.5
};

static cr8r_vec_ft vecft_i64 = {
	.base.size = sizeof(int64_t),
	.new_size = cr8r_default_new_size,
	.resize = cr8r_default_resize,
	.cmp = cr8r_default_cmp_i64,
	.swap = cr8r_default_swap
};

int main(){
	cr8r_hashtbl_t numbers;
	if(!cr8r_hash_init(&numbers, &ft, 8)){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate hashtable\e[0m\n");
		exit(1);
	}
	cr8r_vec removed;
	if(!cr8r_vec_init(&removed, &vecft_i64, 64)){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate vector\e[0m\n");
		exit(1);
	}
	int64_t *rm_it = NULL;
	int64_t n;
	for(n = 0; removed.len < 7; ++n){
		if(numbers.table_b && !rm_it){
			rm_it = numbers.table_b;
		}
		if(rm_it){
			if(rm_it == numbers.table_b && numbers.flags_b[0]&0x100000000ULL){// we are in the first slot and it is occupied
				if(!cr8r_vec_pushr(&removed, &vecft_i64, rm_it)){
					fprintf(stderr, "\e[1;31mERROR: Could not allocate memory\e[0m\n");
					exit(1);
				}
				int64_t tmp = *rm_it;
				cr8r_hash_remove(&numbers, &ft, &tmp);// force re-search for rm_it, to confirm remove_split works
			}else{
				rm_it = cr8r_hash_next(&numbers, &ft, rm_it);
				if((void*)rm_it <= numbers.table_b || (numbers.table_a > numbers.table_b && (void*)rm_it >= numbers.table_a)){// we are done with table_b
					while(numbers.table_b){
						cr8r_hash_get(&numbers, &ft, &n);// force the hash table to finish the incremental move process if needed
					}
					rm_it = NULL;
				}else{
					if(!cr8r_vec_pushr(&removed, &vecft_i64, rm_it)){
						fprintf(stderr, "\e[1;31mERROR: Could not allocate memory\e[0m\n");
						exit(1);
					}
					int64_t tmp = *rm_it;
					cr8r_hash_remove(&numbers, &ft, &tmp);// force re-search for rm_it, to confirm remove_split works
				}
			}
		}
		if(!cr8r_hash_insert(&numbers, &ft, &n, NULL)){
			fprintf(stderr, "\e[1;31mERROR: Could not allocate memory\e[0m\n");
			exit(1);
		}
	}
	for(rm_it = cr8r_hash_next(&numbers, &ft, NULL); rm_it; rm_it = cr8r_hash_next(&numbers, &ft, rm_it)){
		if(!cr8r_vec_pushr(&removed, &vecft_i64, rm_it)){
			fprintf(stderr, "\e[1;31mERROR: Could not allocate memory\e[0m\n");
			exit(1);
		}
		cr8r_hash_delete(&numbers, &ft, rm_it);
	}
	cr8r_hash_destroy(&numbers, &ft);
	cr8r_vec_sort(&removed, &vecft_i64);// TODO: use i64 cmp
	for(int64_t i = 0; i < n; ++i){
		if(i != *(int64_t*)cr8r_vec_get(&removed, &vecft_i64, i)){
			fprintf(stderr, "\e[1;31mERROR: hash insert/remove didn't preserve values!\e[0m\n");
			exit(1);
		}
	}
	fprintf(stderr, "\e[1;32mSuccess: hash insert/remove worked as expected!\e[0m\n");
	cr8r_vec_delete(&removed, &vecft_i64);
}

