#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <crater/avl_check.h>
#include <crater/avl.h>
#include <crater/sla.h>

/*
In this test, we first create an avl tree of uint8_t's node-by-node.
This tree will have all even numbers from 2 to 2*96 inclusive, but they
are inserted out of order.

Then we iterate over the tree using cr8r_avl_last and cr8r_avl_prev to check
it has the correct contents in the correct order.

Then we call cr8r_avl_lower_bound and cr8r_avl_upper_bound for every number on [0, 2*97)
(notice that some of these are not in the tree)
*/

static cr8r_avl_ft avlft;

// Compares a^5 and b^5 mod 97.  x^5 is surjective mod 97, so this is basically
// a consistent random ordering on the numbers [0, 97)
static int cmp_u8_p5m97(const cr8r_base_ft*, const void *_a, const void *_b){
	uint64_t a = *(const uint8_t*)_a, b = *(const uint8_t*)_b;
	a = (a*a)*(a*a)*a%97;
	b = (b*b)*(b*b)*b%97;
	if(a < b){
		return -1;
	}else if(a > b){
		return 1;
	}
	return 0;
}

static uint8_t seen_ents[(98 + 7)/8];

static void record_ents(cr8r_base_ft*, void *node){
	uint8_t n = *(uint8_t*)((cr8r_avl_node*)node)->data;
	if(n > 97){
		n = 97;
	}
	seen_ents[n/8] |= 1ull << (n%8);
}

int main(){
	cr8r_sla sla [[gnu::cleanup(cr8r_sla_delete)]] = {};
	cr8r_sla_init(&sla, offsetof(cr8r_avl_node, data) + sizeof(uint8_t), 97);
	cr8r_avl_ft_init(&avlft, &sla, sizeof(uint8_t), cr8r_default_cmp_u8, NULL, cr8r_default_alloc_sla, cr8r_default_free_sla);
	cr8r_avl_node *root = NULL;
	for(uint64_t n = 50;;){
		uint8_t _n = n;
		if(!cr8r_avl_insert(&root, &_n, &avlft)){
			fprintf(stderr, "\e[1;31mERROR: avl_insert failed somehow!\e[0m\n");
			exit(1);
		}
		n = n/2*5%97*2; // 5 is a generator modulo 97, so repeatedly multiplying any power of 5 mod 97 will generate all numbers besides 0
		if(n == 50){
			break;
		}
	}
	// so now the avl tree should have all the even numbers from 2 to 192 inclusive
	uint8_t n = 96*2;
	for(cr8r_avl_node *it = cr8r_avl_last(root); it; it = cr8r_avl_prev(it)){
		if(n != *(uint8_t*)it->data){
			fprintf(stderr, "\e[1;31mFailure: reverse inorder traversal is not correct!\e[0m\n");
		}
		n -= 2;
	}
	for(n = 0; n < 2; ++n){
		cr8r_avl_node *it = cr8r_avl_lower_bound(root, &n, &avlft);
		if(it){
			fprintf(stderr, "\e[1;31mFailure: avl_lower_bound returned bogus node for n < min (should be NULL)\e[0m\n");
		}
		it = cr8r_avl_upper_bound(root, &n, &avlft);
		if(!it ||*(uint8_t*)it->data != 2){
			fprintf(stderr, "\e[1;31mFailure: avl_upper_bound returned wrong node for %"PRIu8"\e[0m\n", n);
		}
	}
	for(n = 2; n < 96*2; ++n){
		cr8r_avl_node *it = cr8r_avl_lower_bound(root, &n, &avlft);
		if(!it || *(uint8_t*)it->data/2 != n/2){
			fprintf(stderr, "\e[1;31mFailure: avl_lower_bound returned wrong node for %"PRIu8"\e[0m\n", n);
		}
		it = cr8r_avl_upper_bound(root, &n, &avlft);
		if(!it || *(uint8_t*)it->data/2 != n/2 + 1){
			fprintf(stderr, "\e[1;31mFailure: avl_upper_bound returned wrong node for %"PRIu8"\e[0m\n", n);
		}
	}
	for(n = 96*2; n < 97*2; ++n){
		cr8r_avl_node *it = cr8r_avl_lower_bound(root, &n, &avlft);
		if(!it || *(uint8_t*)it->data != 96*2){
			fprintf(stderr, "\e[1;31mFailure: avl_lower_bound returned wrong node for %"PRIu8"\e[0m\n", n);
		}
		it = cr8r_avl_upper_bound(root, &n, &avlft);
		if(it){
			fprintf(stderr, "\e[1;31mFailure: avl_upper_bound returned bogus node for n < min (should be NULL)\e[0m\n");
		}
	}
	for(n = 1; n < 97*2; ++n){
		cr8r_avl_node *it = cr8r_avl_search(root, &n, &avlft);
		if(!it){
			fprintf(stderr, "\e[1;31mFailure: avl_search returned NULL\e[0m\n");
		}else if(n&1){
			uint8_t m = *(uint8_t*)it->data;
			if(m + 1 != n && n + 1 != m){
				fprintf(stderr, "\e[1;31mFailure: avl_search failed\e[0m\n");
			}
		}else if(*(uint8_t*)it->data != n){
			fprintf(stderr, "\e[1;31mFailure: avl_search failed\e[0m\n");
		}
	}
	for(n = 2; n < 97*2; n += 2){
		cr8r_avl_node *it = cr8r_avl_get(root, &n, &avlft);
		if(!it){
			fprintf(stderr, "\e[1;31mFailure: couldn't get node to increase\e[0m\n");
		}else{
			*(uint8_t*)it->data += 29;
			if(!(it = cr8r_avl_increase(it, &avlft, NULL))){
				fprintf(stderr, "\e[1;31mFailure: avl_increase falsely detected duplicate\e[0m\n]");
			}else{
				root = it;
				CR8R_AVL_ASSERT_ALL(root);
			}
		}
	}
	n = 2 + 29;
	for(cr8r_avl_node *it = cr8r_avl_first(root); it; it = cr8r_avl_next(it)){
		if(n != *(uint8_t*)it->data){
			fprintf(stderr, "\e[1;31mFailure: avl_increase broke the tree!\e[0m\n");
		}
		n += 2;
	}
	for(n = 2 + 29; n < 97*2 + 29; n += 2){
		cr8r_avl_node *it = cr8r_avl_get(root, &n, &avlft);
		if(!it){
			fprintf(stderr, "\e[1;31mFailure: couldn't get node to decrease\e[0m\n");
		}else{
			*(uint8_t*)it->data -= 29;
			if(!(it = cr8r_avl_decrease(it, &avlft, NULL))){
				fprintf(stderr, "\e[1;31mFailure: avl_decrease falsely detected duplicate\e[0m\n]");
			}else{
				root = it;
				CR8R_AVL_ASSERT_ALL(root);
			}
		}
	}
	n = 2;
	for(cr8r_avl_node *it = cr8r_avl_first(root); it; it = cr8r_avl_next(it)){
		if(n != *(uint8_t*)it->data){
			fprintf(stderr, "\e[1;31mFailure: avl_decrease broke the tree!\e[0m\n");
		}
		n += 2;
	}
	n = 0;
	for(cr8r_avl_node *it = cr8r_avl_first(root); it; it = cr8r_avl_next(it)){
		*(uint8_t*)it->data = n++;
	}
	avlft.cmp = cmp_u8_p5m97;
	cr8r_avl_reorder(root, &avlft);
	CR8R_AVL_ASSERT_ALL(root);
	n = 0;
	for(cr8r_avl_node *it = cr8r_avl_first(root), *pd = NULL; it; pd = it, it = cr8r_avl_next(it)){
		if(pd && cmp_u8_p5m97(&avlft.base, pd->data, it->data) >= 0){
			fprintf(stderr, "\e[1;31mFailure: avl_reorder did not restore order!\e[0m\n");
		}
		++n;
	}
	if(n != 96){
		fprintf(stderr, "\e[1;31mFailure: avl_reorder'd tree has %"PRIu8" nodes instead of 96\e[0m\n", n);
	}
	if((n = *(uint8_t*)cr8r_avl_first(root)->data)){
		fprintf(stderr, "\e[1;31mFailure: avl_reorder made %"PRIu8" the first element instead of 0\e[0m\n", n);
	}
	avlft.free = record_ents;
	cr8r_avl_delete(root, &avlft);
	for(uint8_t n = 0; n < 96; ++n){
		if(!(seen_ents[n/8] & (1ull << (n%8)))){
			fprintf(stderr, "\e[1;31mFailure: avl_delete missed node %"PRIu8"!\e[0m\n", n);
		}
	}
	n = 97;
	if(seen_ents[n/8] & (1ull << (n%8))){
		fprintf(stderr, "\e[1;31mFailure: avl_delete encountered some extradimensional node :O\e[0m\n");
	}
}

