#include <stdlib.h>
#include <string.h>

#include <crater/pheap.h>

cr8r_pheap_node *cr8r_pheap_new(void *key, cr8r_pheap_ft *ft, cr8r_pheap_node *first_child, cr8r_pheap_node *sibling, cr8r_pheap_node *parent){
	void *res = ft->alloc ? ft->alloc(&ft->base) : NULL;
	if(res){
		memcpy(res, key, ft->base.size);
		memcpy(res + ft->base.size, &(cr8r_pheap_node){.first_child = first_child, .sibling = sibling, .parent = parent}, sizeof(cr8r_pheap_node));
	}
	return res + ft->base.size;
}

void *cr8r_pheap_top(cr8r_pheap_node *r, cr8r_pheap_ft *ft){
	return r ? CR8R_OUTER_S(r, ft) : NULL;
}

cr8r_pheap_node *cr8r_pheap_root(cr8r_pheap_node *n){
	if(!n){
		return NULL;
	}
	while(n->parent){
		n = n->parent;
	}
	return n;
}

cr8r_pheap_node *cr8r_pheap_meld(cr8r_pheap_node *a, cr8r_pheap_node *b, cr8r_pheap_ft *ft){
	if(!a){
		return b;
	}else if(!b){
		return a;
	}else if(ft->cmp(&ft->base, CR8R_OUTER_S(a, ft), CR8R_OUTER_S(b, ft)) > 0){
		a->sibling = b->first_child;
		b->first_child = a;
		a->parent = b;
		return b;
	}
	b->sibling = a->first_child;
	a->first_child = b;
	b->parent = a;
	return a;
}

cr8r_pheap_node *cr8r_pheap_push(cr8r_pheap_node *r, void *key, cr8r_pheap_ft *ft){
	cr8r_pheap_node *n = cr8r_pheap_new(key, ft, NULL, NULL, NULL);
	return n ? cr8r_pheap_meld(r, n, ft) : NULL;
}

/// helper function for cr8r_pheap_pop.  Uses mergesort to combine a cr8r_pheap_node's children
/// linked list into one pairing heap.  Since the length of the list of children is
/// not stored, this function accepts a recursion depth and is called by another helper
/// function (cr8r_pheap_merge_exponential) with depth 0, 0, 1, 2, 3, 4, 5, 6, ... until all
/// children are merged.  If the list is empty, nothing is done.  If the depth is 0,
/// the first child in the remaining part of the list is removed and returned (1 node is sorted).
static cr8r_pheap_node *cr8r_pheap_merge_binary(cr8r_pheap_node **n, size_t depth, cr8r_pheap_ft *ft){
	cr8r_pheap_node *a, *b;
	if(!*n){
		return NULL;
	}else if(!depth){
		a = *n;
		*n = (*n)->sibling;
		a->sibling = NULL;
		a->parent = NULL;
		return a;
	}
	a = cr8r_pheap_merge_binary(n, depth - 1, ft);
	b = cr8r_pheap_merge_binary(n, depth - 1, ft);
	return cr8r_pheap_meld(a, b, ft);
}

/// helper function for pheap_pop.  Uses mergesort to combine a pheap_node's children
/// linked list into one pairing heap.
static cr8r_pheap_node *cr8r_pheap_merge_exponential(cr8r_pheap_node **n, cr8r_pheap_ft *ft){
	cr8r_pheap_node *a = cr8r_pheap_merge_binary(n, 0, ft);
	for(size_t depth = 0; *n; ++depth){
		a = cr8r_pheap_meld(a, cr8r_pheap_merge_binary(n, depth, ft), ft);
	}
	return a;
}

cr8r_pheap_node *cr8r_pheap_pop(cr8r_pheap_node **r, cr8r_pheap_ft *ft){
	if(!*r){
		return NULL;
	}
	cr8r_pheap_node *ret = *r;
	*r = cr8r_pheap_merge_exponential(&(*r)->first_child, ft);
	return ret;
}

cr8r_pheap_node *cr8r_pheap_decreased_key(cr8r_pheap_node *n, cr8r_pheap_ft *ft){
	if(!n){
		return NULL;
	}
	cr8r_pheap_node *p = n->parent;
	if(!p || ft->cmp(&ft->base, CR8R_OUTER_S(p, ft), CR8R_OUTER_S(n, ft)) <= 0){
		return cr8r_pheap_root(p);
	}else if(p->first_child == n){
		p->first_child = n->sibling;
	}else for(cr8r_pheap_node *s = p->first_child;; s = s->sibling){
		if(s->sibling == n){
			s->sibling = n->sibling;
			break;
		}
	}
	n->sibling = NULL;
	n->parent = NULL;
	return cr8r_pheap_meld(cr8r_pheap_root(p), n, ft);
}

void cr8r_pheap_delete(cr8r_pheap_node *r, cr8r_pheap_ft *ft){
	if(!ft->free){
		return;
	}
	for(cr8r_pheap_node *t; r;){
		while(r->sibling){
			cr8r_pheap_delete(r->sibling->first_child, ft);
			t = r->sibling;
			r->sibling = t->sibling;
			ft->free(&ft->base, t);
		}
		t = r;
		r = r->first_child;
		ft->free(&ft->base, t);
	}
}

