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
	/*fprintf(stderr, "\e[1;34m(");
	for(uint64_t i = 0; i < iseq->len; ++i){
		fprintf(stderr, i == iseq->len - 1 ? "%"PRIu64")\e[0m\n" : "%"PRIu64",", *(uint64_t*)cr8r_vec_get((cr8r_vec*)iseq, iseq_ft, i));
	}*/
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
	fprintf(stderr, "\e[1;34mTesting AVL tree with all possible sequences of 7 inserts/removals\n(this could take a minute or two)...\e[0m\n");
	cr8r_vec iseq, rseq;
	cr8r_vec_ft ft = cr8r_vecft_u64;
	ft.base.data = &rseq;
	if(!cr8r_vec_init(&iseq, &ft, 7)){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate iseq vector.\e[0m\n");
		exit(1);
	}
	if(!cr8r_vec_init(&rseq, &ft, 7)){
		fprintf(stderr, "\e[1;31mERROR: Could not allocate rseq vector.\e[0m\n");
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

