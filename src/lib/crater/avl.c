#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <crater/avl_check.h>
#include <crater/avl.h>

static cr8r_avl_node *cr8r_avl_insert_recursive(cr8r_avl_node *r, void *key, cr8r_avl_ft *ft);
inline static cr8r_avl_node *cr8r_avl_insert_rebalance_r(cr8r_avl_node *p, cr8r_avl_node *n);
inline static cr8r_avl_node *cr8r_avl_insert_rebalance_l(cr8r_avl_node *p, cr8r_avl_node *n);
static cr8r_avl_node *cr8r_avl_insert_retrace(cr8r_avl_node *n);
inline static cr8r_avl_node *cr8r_avl_remove_rebalance_r(cr8r_avl_node *p);
inline static cr8r_avl_node *cr8r_avl_remove_rebalance_l(cr8r_avl_node *p);
static cr8r_avl_node *cr8r_avl_remove_retrace(cr8r_avl_node *n);
inline static cr8r_avl_node *cr8r_avl_remove_trunk(cr8r_avl_node *n, cr8r_avl_ft *ft);
inline static cr8r_avl_node *cr8r_avl_rotate_r(cr8r_avl_node *n);
inline static cr8r_avl_node *cr8r_avl_rotate_l(cr8r_avl_node *n);

inline static void cr8r_avl_swap_data(cr8r_avl_node *a, cr8r_avl_node *b, uint64_t size);
inline static void cr8r_avl_sift_down_bounded(cr8r_avl_node *r, cr8r_avl_node *u, cr8r_avl_ft *ft);
inline static void cr8r_avl_reorder_recursive(cr8r_avl_node *r, cr8r_avl_ft *ft);
inline static void cr8r_avl_reorder_retrace(cr8r_avl_node *n);
inline static cr8r_avl_node *cr8r_avl_reorder_relink(cr8r_avl_node *n);


bool cr8r_avl_ft_init(cr8r_avl_ft *ft,
	void *data, uint64_t size,
	int (*cmp)(const cr8r_base_ft*, const void*, const void*),
	int (*add)(cr8r_base_ft*, void*, void*),
	void *(*alloc)(cr8r_base_ft*),
	void (*free)(cr8r_base_ft*, void*)
){
	if(!cmp || !alloc || !free){
		return 0;
	}
	ft->base.data = data;
	ft->base.size = size;
	ft->cmp = cmp;
	ft->add = add;
	ft->alloc = alloc;
	ft->free = free;
	return 1;
}

bool cr8r_avl_ft_initsla(cr8r_avl_ft *ft,
	cr8r_sla *sla, uint64_t size, uint64_t reserve,
	int (*cmp)(const cr8r_base_ft*, const void*, const void*),
	int (*add)(cr8r_base_ft*, void*, void*)
){
	if(!cmp || !sla || !cr8r_sla_init(sla, offsetof(cr8r_avl_node, data) + size, reserve)){
		return 0;
	}
	ft->base.data = sla;
	ft->base.size = size;
	ft->cmp = cmp;
	ft->add = add;
	ft->alloc = cr8r_default_alloc_sla;
	ft->free = cr8r_default_free_sla;
	return 1;
}

void *cr8r_default_alloc_sla(cr8r_base_ft *ft){
	return cr8r_sla_alloc(ft->data);
}

void cr8r_default_free_sla(cr8r_base_ft *ft, void *p){
	cr8r_sla_free(ft->data, p);
}


cr8r_avl_node *cr8r_avl_next(cr8r_avl_node *n){
	if(n->right){
		return cr8r_avl_first(n->right);
	}
	cr8r_avl_node *s = n;//inorder successor of n
	while(s->parent && s->parent->left != s){
		s = s->parent;
	}
	return s->parent;
}

cr8r_avl_node *cr8r_avl_prev(cr8r_avl_node *n){
	if(n->left){
		return cr8r_avl_last(n->left);
	}
	cr8r_avl_node *p = n;//inorder predecessor of n
	while(p->parent && p->parent->right != p){
		p = p->parent;
	}
	return p->parent;
}

cr8r_avl_node *cr8r_avl_new(void *key, cr8r_avl_node *left, cr8r_avl_node *right, cr8r_avl_node *parent, char balance, cr8r_avl_ft *ft){
	cr8r_avl_node *ret = ft->alloc(&ft->base);
	if(ret){
		*ret = (cr8r_avl_node){left, right, parent, balance};
		memcpy(ret->data, key, ft->base.size);
	}
	return ret;
}

cr8r_avl_node *cr8r_avl_root(cr8r_avl_node *n){
	while(n && n->parent){
		n = n->parent;
	}
	return n;
}

cr8r_avl_node *cr8r_avl_get(cr8r_avl_node *r, void *key, cr8r_avl_ft *ft){
	if(!r){
		return NULL;
	}
	int ord = ft->cmp(&ft->base, r->data, key);
	if(ord < 0){
		return cr8r_avl_get(r->right, key, ft);
	}else if(ord > 0){
		return cr8r_avl_get(r->left, key, ft);
	}//otherwise r->data "==" key
	return r;
}

cr8r_avl_node *cr8r_avl_search_dups(cr8r_avl_node *r, void *key, cr8r_avl_ft *ft, int *is_duplicate){
	if(!r){
		if(is_duplicate){
			*is_duplicate = 0;
		}
		return NULL;
	}
	int found_dup = 0;
	while(1){
		int ord = ft->cmp(&ft->base, r->data, key);
		if(ord < 0){
			if(!r->right){
				break;
			}
			r = r->right;
		}else if(ord > 0){
			if(!r->left){
				break;
			}
			r = r->left;
		}else{
			found_dup = 1;
			if(r->balance < 0){
				if(!r->right){
					break;
				}
				r = r->right;
			}else if(!r->left){
				break;
			}else{
				r = r->left;
			}
		}
	}
	if(is_duplicate){
		*is_duplicate = found_dup;
	}
	return r;
}

cr8r_avl_node *cr8r_avl_search(cr8r_avl_node *r, void *key, cr8r_avl_ft *ft){
	if(!r){
		return NULL;
	}
	int ord = ft->cmp(&ft->base, r->data, key);
	if(ord < 0){
		return r->right ? cr8r_avl_search(r->right, key, ft) : r;
	}else if(ord > 0){
		return r->left ? cr8r_avl_search(r->left, key, ft) : r;
	}
	return r;
}

cr8r_avl_node *cr8r_avl_lower_bound(cr8r_avl_node *r, void *key, cr8r_avl_ft *ft){
	cr8r_avl_node *ret = NULL;
	if(!r){
		return NULL;
	}//find largest element l of r such that l <= key
	int ord = ft->cmp(&ft->base, r->data, key);
	if(ord < 0){
		ret = cr8r_avl_lower_bound(r->right, key, ft);
		return ret ? ret : r;
	}else if(ord > 0){
		return cr8r_avl_lower_bound(r->left, key, ft);
	}//otherwise r->data "==" key
	return r;
}

cr8r_avl_node *cr8r_avl_upper_bound(cr8r_avl_node *r, void *key, cr8r_avl_ft *ft){
	cr8r_avl_node *ret = NULL;
	if(!r){
		return NULL;
	}//find smallest element u of r such that r > key
	int ord = ft->cmp(&ft->base, r->data, key);
	if(ord < 0){
		return cr8r_avl_upper_bound(r->right, key, ft);
	}else if(ord > 0){
		ret = cr8r_avl_upper_bound(r->left, key, ft);
		return ret ? ret : r;
	}//otherwise r->data "==" key
	return cr8r_avl_next(r);
}

cr8r_avl_node *cr8r_avl_first(cr8r_avl_node *r){
	while(r && r->left){
		r = r->left;
	}
	return r;
}

cr8r_avl_node *cr8r_avl_last(cr8r_avl_node *r){
	while(r && r->right){
		r = r->right;
	}
	return r;
}

int cr8r_avl_insert(cr8r_avl_node **r, void *key, cr8r_avl_ft *ft){
	cr8r_avl_node *t = cr8r_avl_insert_recursive(*r, key, ft);
	CR8R_AVL_ASSERT_ALL(*r);
	if(t){
		*r = t;
	}
	return !!t;
}

cr8r_avl_node *cr8r_avl_insert_recursive(cr8r_avl_node *r, void *key, cr8r_avl_ft *ft){
	if(!r){
		return cr8r_avl_new(key, NULL, NULL, NULL, 0, ft);
	}
	int ord = ft->cmp(&ft->base, r->data, key);
	if(ord < 0){
		if(r->right){
			return cr8r_avl_insert_recursive(r->right, key, ft);
		}//otherwise insert on the right side
		r->right = cr8r_avl_new(key, NULL, NULL, r, 0, ft);
		if(!r->right){//allocation failed
			return NULL;
		}
		return cr8r_avl_insert_retrace(r->right);//rebalance
	}else if(ord > 0){
		if(r->left){
			return cr8r_avl_insert_recursive(r->left, key, ft);
		}//otherwise insert on the left side
		r->left = cr8r_avl_new(key, NULL, NULL, r, 0, ft);
		if(!r->left){//allocation failed
			return NULL;
		}
		return cr8r_avl_insert_retrace(r->left);//rebalance
	}
	return NULL;
}

int cr8r_avl_insert_update(cr8r_avl_node **r, void *key, cr8r_avl_ft *ft){
	if(!*r){
		*r = cr8r_avl_new(key, NULL, NULL, NULL, 0, ft);
		return *r ? CR8R_AVL_INSERTED : 0;
	}
	for(cr8r_avl_node *t = *r;;){
		int ord = ft->cmp(&ft->base, t->data, key);
		if(ord < 0){
			if(t->right){
				t = t->right;
			}else{
				if(!(t->right = cr8r_avl_new(key, NULL, NULL, t, 0, ft))){
					return 0;
				}
				*r = cr8r_avl_insert_retrace(t->right);
				CR8R_AVL_ASSERT_ALL(*r);
				return CR8R_AVL_INSERTED;
			}
		}else if(ord > 0){
			if(t->left){
				t = t->left;
			}else{
				if(!(t->left = cr8r_avl_new(key, NULL, NULL, t, 0, ft))){
					return 0;
				}
				*r = cr8r_avl_insert_retrace(t->left);
				CR8R_AVL_ASSERT_ALL(*r);
				return CR8R_AVL_INSERTED;
			}
		}else{
			return ft->add(&ft->base, t->data, key) ? CR8R_AVL_UPDATED : 0;
		}
	}
}

int cr8r_avl_remove(cr8r_avl_node **r, void *key, cr8r_avl_ft *ft){
	cr8r_avl_node *n = cr8r_avl_get(*r, key, ft);
	CR8R_AVL_ASSERT_ALL(*r);
	if(n){
		*r = cr8r_avl_remove_node(n, ft);
	}
	return !!n;
}

cr8r_avl_node *cr8r_avl_remove_node(cr8r_avl_node *n, cr8r_avl_ft *ft){
	cr8r_avl_node *s = cr8r_avl_next(n);
	if((!n->left) || (!n->right)){//no need to swap with a scapegoat
		return cr8r_avl_remove_trunk(n, ft);
	}
	//swap all of n and s's parent, left, right, and balance fields
	//it would be easier to swap the data fields but that would invalidate iterators to s
	cr8r_avl_node t = *s;//t is an automatic storage variable that holds a copy of *s
	if((s->left = n->left)){
		s->left->parent = s;
	}
	if((s->right = n->right)){
		s->right->parent = s;
	}
	if((s->parent = n->parent)){
		*(s->parent->left == n ? &s->parent->left : &s->parent->right) = s;//dereference(reference) allows the ternary operator to be an lvalue
	}
	s->balance = n->balance;
	n->left = NULL;//s is n's inorder successor and n has a right child so s does not have a left child.
	if((n->right = t.right)){
		n->right->parent = n;
	}
	if((n->parent = t.parent)){
		*(n->parent->left == s ? &n->parent->left : &n->parent->right) = n;
	}
	n->balance = t.balance;
	if(t.parent == n){//s was n's child.  In this case, these pointers loop back and need to be fixed.
		s->right = n;
		n->parent = s;
		n->right = t.right;
	}
	return cr8r_avl_remove_trunk(n, ft);
}

cr8r_avl_node *cr8r_avl_remove_trunk(cr8r_avl_node *n, cr8r_avl_ft *ft){
	cr8r_avl_node *p = n->parent, *c = n->left ? n->left : n->right, *r;//Parent, Child
	if(!p){
		if(c){
			c->parent = NULL;
		}
		ft->free(&ft->base, n);
		return c;//This does not indicate an error if NULL.
	}
	if(p->left == n){
		r = cr8r_avl_remove_retrace(n);//r is the root now
		n->parent->left = c;
		if(c){
			c->parent = n->parent;
		}
		ft->free(&ft->base, n);
		return r;
	}//p->right == n
	r = cr8r_avl_remove_retrace(n);//r is the root
	n->parent->right = c;
	if(c){
		c->parent = n->parent;
	}
	ft->free(&ft->base, n);
	return r;
}

cr8r_avl_node *cr8r_avl_insert_retrace(cr8r_avl_node *n){
	cr8r_avl_node *p = n->parent;
	if(!p){
		return n;
	}
	if(n == p->right){//+2 cases
		switch(p->balance){
			case -1://height change absorbed
			p->balance = 0;
			return cr8r_avl_root(p);
			case 0:
			p->balance = 1;
			return cr8r_avl_insert_retrace(p);
		}//default: p->balance == 1
		return cr8r_avl_insert_rebalance_r(p, n);//height change will be absorbed
	}//otherwise n == p->left
	switch(p->balance){//-2 cases
		case 1://height change absorbed
		p->balance = 0;
		return cr8r_avl_root(p);
		case 0:
		p->balance = -1;
		return cr8r_avl_insert_retrace(p);
	}//default: p->balance == -1
	return cr8r_avl_insert_rebalance_l(p, n);//height change will be absorbed
}

cr8r_avl_node *cr8r_avl_insert_rebalance_r(cr8r_avl_node *p, cr8r_avl_node *n){
	if(n->balance == 1){//right-right case: only one rotation needed
		cr8r_avl_rotate_r(p);//note n becomes the parent of p
		p->balance = n->balance = 0;
		return cr8r_avl_root(n);
	}//otherwise right-left case: two rotations are required and the balance factors have 3 cases
	cr8r_avl_rotate_l(n);
	cr8r_avl_rotate_r(p);//note p, n->left, n change roles from parent, right child, right-left grandchild to left child, parent, right child
	n->balance = +(n->parent->balance == -1);//n, now the right child of n->parent, its old left child, has a balance factor of 1 if the child was -1, else 0
	p->balance = -(n->parent->balance == 1);//p, now the right child of n->parent, has a balance factor of -1 if n->parent was 1, else 0
	n->parent->balance = 0;//if this is still confusing, try drawing out all of the possible trees with root +2 and right child +/-1 with irrelevant subtrees only marked as heights.
	return cr8r_avl_root(n->parent);//there are 4 cases and I will try to put them in a pdf somewhere if I can figure out how to draw it on a computer.
}

cr8r_avl_node *cr8r_avl_insert_rebalance_l(cr8r_avl_node *p, cr8r_avl_node *n){//mirror image of cr8r_avl_insert_rebalance_r
	if(n->balance == -1){
		cr8r_avl_rotate_l(p);
		p->balance = n->balance = 0;
		return cr8r_avl_root(n);
	}
	cr8r_avl_rotate_r(n);
	cr8r_avl_rotate_l(p);
	n->balance = -(n->parent->balance == 1);
	p->balance = +(n->parent->balance == -1);//unary + for symmetry
	n->parent->balance = 0;
	return cr8r_avl_root(n->parent);
}

cr8r_avl_node *cr8r_avl_remove_retrace(cr8r_avl_node *n){
	cr8r_avl_node *p = n->parent;
	if(!p){
		return n;
	}
	if(n == p->left){//+2 cases
		switch(p->balance){
			case -1:
			p->balance = 0;
			return cr8r_avl_remove_retrace(p);
			case 0://change in height absorbed
			p->balance = 1;
			return cr8r_avl_root(p);
		}//default: p->balance == 1
		return cr8r_avl_remove_rebalance_l(p);
	}//otherwise n == p->right
	switch(p->balance){//-2 cases
		case 1:
		p->balance = 0;
		return cr8r_avl_remove_retrace(p);
		case 0://change in height absorbed
		p->balance = -1;
		return cr8r_avl_root(p);
	}//default: p->balance == -1
	return cr8r_avl_remove_rebalance_r(p);
}

cr8r_avl_node *cr8r_avl_remove_rebalance_l(cr8r_avl_node *p){//do not try to understand this until you understand cr8r_avl_insert_rebalance_r
	cr8r_avl_node *n = p->right;//If we've become off balance by a left REMOVAL there must be a right child
	if(n->balance != -1){//not the right-left case
		cr8r_avl_rotate_r(p);//nb. the pointers n and p point to the same nodes but now n is the parent
		p->balance -= n->balance--;//if n is +1, n and p become 0, otherwise n is 0 and they become -1 and +1
		return n->balance ? cr8r_avl_root(n) : cr8r_avl_remove_retrace(n);//if n->balance becomes 0 by removal we still have height decrease to propagate
	}//right-left case.  This is exactly the same as insertion whereas the previous part included a right-even case not present in insertion
	cr8r_avl_rotate_l(n);
	cr8r_avl_rotate_r(p);
	p->balance = -(n->parent->balance == 1);
	n->balance = +(n->parent->balance == -1);
	n->parent->balance = 0;
	return cr8r_avl_remove_retrace(n->parent);
}

cr8r_avl_node *cr8r_avl_remove_rebalance_r(cr8r_avl_node *p){//mirror image of cr8r_avl_remove_rebalance_l
	cr8r_avl_node *n = p->left;
	if(n->balance != 1){
		cr8r_avl_rotate_l(p);
		p->balance -= n->balance++;//if n is -1, n and p become 0, otherwise n is 0 and they become +1 and -1.  Note the -= needn't be flipped since -- was
		return n->balance ? cr8r_avl_root(n) : cr8r_avl_remove_retrace(n);
	}
	cr8r_avl_rotate_r(n);
	cr8r_avl_rotate_l(p);
	p->balance = +(n->parent->balance == -1);
	n->balance = -(n->parent->balance == 1);
	n->parent->balance = 0;
	return cr8r_avl_remove_retrace(n->parent);
}

cr8r_avl_node *cr8r_avl_rotate_l(cr8r_avl_node *n){//does not update balance factors because the caller knows better
	cr8r_avl_node *l = n->left;//assume existence of swapped node
	l->parent = n->parent;
	if(n->parent){
		if(n->parent->left == n){
			n->parent->left = l;
		}else{//n->parent->right == n
			n->parent->right = l;
		}
	}
	n->parent = l;
	n->left = l->right;
	if(n->left){
		n->left->parent = n;
	}
	l->right = n;
	return l;
}

cr8r_avl_node *cr8r_avl_rotate_r(cr8r_avl_node *n){
	cr8r_avl_node *r = n->right;//assume existence of swapped node
	r->parent = n->parent;
	if(n->parent){
		if(n->parent->left == n){
			n->parent->left = r;
		}else{//n->parent->right == n
			n->parent->right = r;
		}
	}
	n->parent = r;
	n->right = r->left;
	if(n->right){
		n->right->parent = n;
	}
	r->left = n;
	return r;
}

void cr8r_avl_delete(cr8r_avl_node *r, cr8r_avl_ft *ft){
	if(!r)return;
	cr8r_avl_delete(r->left, ft);
	cr8r_avl_delete(r->right, ft);
	ft->free(&ft->base, r);
}

cr8r_avl_node *cr8r_avl_attach(cr8r_avl_node *r, cr8r_avl_node *n, cr8r_avl_ft *ft, int *is_duplicate){
	if(!r){
		if(is_duplicate){
			*is_duplicate = 0;
		}
		return n;
	}
	r = cr8r_avl_search_dups(r, n->data, ft, is_duplicate);
	int ord = ft->cmp(&ft->base, r->data, n->data);
	if(ord > 0){
		r->left = n;
	}else if(ord < 0 || r->balance < 0){
		r->right = n;
	}else{
		r->left = n;
	}
	n->parent = r;
	return cr8r_avl_insert_retrace(n);
}

cr8r_avl_node *cr8r_avl_attach_exclusive(cr8r_avl_node *r, cr8r_avl_node *n, cr8r_avl_ft *ft){
	if(!r){
		return n;
	}
	r = cr8r_avl_search(r, n->data, ft);
	int ord = ft->cmp(&ft->base, r->data, n->data);
	if(!ord){
		return NULL;
	}
	if(ord > 0){
		r->left = n;
	}else{
		r->right = n;
	}
	n->parent = r;
	return cr8r_avl_insert_retrace(n);
}

void cr8r_default_free_pass(cr8r_base_ft *ft, void *p){}

cr8r_avl_node *cr8r_avl_detach(cr8r_avl_node *n, cr8r_avl_ft *ft){
	cr8r_avl_ft ft_cpy = *ft;
	ft_cpy.free = cr8r_default_free_pass;
	cr8r_avl_node *r = cr8r_avl_remove_node(n, &ft_cpy);
	n->left = n->right = n->parent = NULL;
	n->balance = 0;
	return r;
}

cr8r_avl_node *cr8r_avl_decrease(cr8r_avl_node *n, cr8r_avl_ft *ft, int *is_duplicate){
	cr8r_avl_node *p = cr8r_avl_prev(n);
	int ord;
	if(!p || (ord = ft->cmp(&ft->base, p->data, n->data)) < 0){
		if(is_duplicate){
			*is_duplicate = 0;
		}
		return cr8r_avl_root(n);
	}else if(!ord){
		if(is_duplicate){
			*is_duplicate = 1;
		}
		return cr8r_avl_root(n);
	}
	p = cr8r_avl_detach(n, ft);
	return cr8r_avl_attach(p, n, ft, is_duplicate);
}

cr8r_avl_node *cr8r_avl_increase(cr8r_avl_node *n, cr8r_avl_ft *ft, int *is_duplicate){
	cr8r_avl_node *s = cr8r_avl_next(n);
	int ord;
	if(!s || (ord = ft->cmp(&ft->base, s->data, n->data)) > 0){
		if(is_duplicate){
			*is_duplicate = 0;
		}
		return cr8r_avl_root(n);
	}else if(!ord){
		if(is_duplicate){
			*is_duplicate = 1;
		}
		return cr8r_avl_root(n);
	}
	s = cr8r_avl_detach(n, ft);
	return cr8r_avl_attach(s, n, ft, is_duplicate);
}

cr8r_avl_node *cr8r_avl_first_post(cr8r_avl_node *r){
	while(1){
		if(r->left){
			r = r->left;
		}else if(r->right){
			r = r->right;
		}else{
			return r;
		}
	}
}

cr8r_avl_node *cr8r_avl_next_post(cr8r_avl_node *n){
	return n->parent && n == n->parent->left && n->parent->right ? cr8r_avl_first_post(n->parent->right) : n->parent;
}

void cr8r_avl_swap_data(cr8r_avl_node *a, cr8r_avl_node *b, uint64_t size){
	char tmp[size];
	memcpy(tmp, a->data, size);
	memcpy(a->data, b->data, size);
	memcpy(b->data, tmp, size);
}

void cr8r_avl_sift_down(cr8r_avl_node *r, cr8r_avl_ft *ft){
	cr8r_avl_sift_down_bounded(r, NULL, ft);
}

// sift down for a partially max heap / partially bst avl tree, where u is the first node (in inorder traversal order)
// in the bst portion (and also an upper bound for the max heap under ft->cmp).
// When u is NULL, this decays to sift down for a fully max heap avl tree.
// Partially max heap / partially bst avl trees only arise during execution of avl_reorder,
// and that function does some extra pointer juggling to ensure that only checking right children for u and stopping once
// it is found is correct; see that function for more info
void cr8r_avl_sift_down_bounded(cr8r_avl_node *r, cr8r_avl_node *u, cr8r_avl_ft *ft){
	cr8r_avl_node *max_child;
	while(1){
		if(r->right && r->right != u){
			if(r->left){
				max_child = ft->cmp(&ft->base, r->left->data, r->right->data) >= 0 ? r->left : r->right;
			}else{
				max_child = r->right;
			}
		}else if(r->left){
			max_child = r->left;
		}else{
			break;
		}
		if(ft->cmp(&ft->base, max_child->data, r->data) <= 0){
			break;
		}
		cr8r_avl_swap_data(max_child, r, ft->base.size);
		r = max_child;
	}
}

void cr8r_avl_heapify(cr8r_avl_node *r, cr8r_avl_ft *ft){
	for(r = cr8r_avl_first_post(r); r; r = cr8r_avl_next_post(r)){
		if(r->left || r->right){
			cr8r_avl_sift_down(r, ft);
		}
	}
}

cr8r_avl_node *cr8r_avl_heappop_node(cr8r_avl_node **r, cr8r_avl_ft *ft){
	if(!*r){
		return NULL;
	}
	cr8r_avl_node *s = *r;
	while(1){
		if(s->balance == -1){
			s = s->left;
		}else if(s->right){
			s = s->right;
		}else{
			break;
		}
	}
	if(s == *r){
		*r = NULL;
		return s;
	}
	for(cr8r_avl_node *u = s; u != *r; u = u->parent){
		signed char db = u->parent->left == u ? 1 : -1;
		u->parent->balance += db;
		if(u->parent->balance){
			break;
		}
	}
	if(s->parent->left == s){
		s->parent->left = NULL;
	}else{
		s->parent->right = NULL;
	}
	s->parent = NULL;//(*r)->parent must be NULL
	if((s->left = (*r)->left)){
		s->left->parent = s;
	}
	if((s->right = (*r)->right)){
		s->right->parent = s;
	}
	s->balance = (*r)->balance;
	CR8R_AVL_ASSERT_ALL(s);
	(*r)->left = (*r)->right = NULL;
	(*r)->balance = 0;
	cr8r_avl_node *res = *r;
	*r = s;
	cr8r_avl_sift_down(s, ft);
	return res;
}

void cr8r_avl_reorder(cr8r_avl_node *r, cr8r_avl_ft *ft){
	cr8r_avl_heapify(r, ft);
	cr8r_avl_reorder_recursive(r, ft);
}

void cr8r_avl_reorder_recursive(cr8r_avl_node *r, cr8r_avl_ft *ft){
	if(!r){
		return;
	}
	// in this function, r is initially fully a max heap, but as we proceed, we convert
	// it to a bst in reverse inorder order.  We do this by repeatedly swapping the data
	// in the root of the max heap portion with the first unvisited node in reverse inorder order.
	// Then we sift down the root of the max heap to preserve the max heap invariant on the
	// heap portion of the avl tree.  The bst portion trivially has its bst invariant satisfied,
	// and we never break the avl invariants because we never actually change the shape of the tree.
	// HOWEVER, if leave the links in the tree alone, then the max heap region and the bst region
	// are not "nice": in particular if the node we are on in reverse inorder traversal has a left subtree,
	// then we have visited it and its right subtree and when we call sift down it would have to
	// determine if it ever ran into u or one of u's reverse inorder predecessors and skip any such nodes.
	// To avoid this, we use sift_down_bounded and adjust the links in the tree as we go
	for(cr8r_avl_node *u = cr8r_avl_last(r); u != r;){
		cr8r_avl_swap_data(u, r, ft->base.size);
		cr8r_avl_sift_down_bounded(r, u, ft);
		if(u->left){
			// if u has a left subtree, the next node in reverse inorder is cr8r_avl_first(u->left),
			// but we want sift_down_bounded to skip u and its reverse inorder predecessors (ie the nodes
			// in the bst portion of the avl tree).  Therefore, we find the FIRST ancestor of u where u is in its right subtree,
			// and we change its right pointer to point to u->left instead.  This ancestor is guaranteed to exist because
			// u is a right descendent of the root (in a reverse inorder traversal this is true until we hit the root, so once we
			// hit the root we force it to continue to be true by recursing on the left subtree, see below).
			// Changing the first left ancestor of u (first ancestor where u is a right descendent) so its right child is u->left
			// breaks the structure of the tree, but we will fix it later.
			// Notice that for ancestors where u is in their left subtree, they are reverse inorder predecessors of u, so
			// they have already been visited.
			cr8r_avl_reorder_retrace(u->left);
			u = cr8r_avl_last(u->left);
		}else{
			// find the first left ancestor of u (which must itself be either the root or in the right subtree of the root, since u is in
			// the right subtree of the root).  Since u has no left subtree, this is u's successor in reverse inorder traversal.
			// We also fix this first left ancestor so that its right child pointer points at its descendent again and not u.
			u = cr8r_avl_reorder_relink(u);
		}
	}
	// if u == r, we have reached the root node.  At this point, the top of the heap and the current node in reverse inorder traversal
	// are the same, so we don't actually have to call avl_swap_data here.
	// We also don't need to temporarily set r->left->parent to NULL, because this recursive call will only look for left ancestors of
	// u for u in the right subtree of r->left, so the left ancestor search will always terminate at or before r->left.
	// Then the recursive call will recurse to r->left->left if needed and so on.
	cr8r_avl_reorder_recursive(r->left, ft);
}

// given n the nonempty left subtree of the just-filled current reverse inorder traversal node u,
// find the first left ancestor of n (the first ancestor where n is a right descendent),
// and change this ancestor's right link to point at n instead.  Note that u == n->parent, and the
// original shape of the tree can be recovered later: when we walk up from u, any ancestor that is
// a left child of its parent will still have the correct parent pointer and its parent will still have
// it as a left child.  However, the first ancestor that is a right child of its parent will have its parent
// pointer correct but its parent's right child will point to n instead of it.
// this might fail to find a left ancestor, but recursing on the left subtree when u reaches the root ensures
// the root is always a left ancestor.  This ensures calling sift_down_bounded on the root with u as the
// bound will see all nodes after u in the reverse inorder traversal of the original tree: when u has a left
// subtree, its entire left subtree is after it in this traversal, and in particular the rightmost element of
// its left subtree is its successor in the traversal.  Also, any time we visit a node on the left edge of the
// left subtree, we update the first left ancestor to point at its left subtree as well, if any.
void cr8r_avl_reorder_retrace(cr8r_avl_node *n){
	cr8r_avl_node *a = n;
	while(a == a->parent->left){
		a = a->parent;
	}
	a->parent->right = n;
}

// when u does not have a left child, its reverse inorder successor will instead be its first left ancestor.
// of course, this is also the node whose right child pointer we clobbered when first moving from any
// ancestor of u before its first left ancestor, so now is when we fix it up.
cr8r_avl_node *cr8r_avl_reorder_relink(cr8r_avl_node *n){
	while(n == n->parent->left){
		n = n->parent;
	}
	n->parent->right = n;
	return n->parent;
}

