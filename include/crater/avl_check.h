#pragma once

#include <crater/avl.h>

inline static int cr8r_avl_check_links(cr8r_avl_node *n){
	if(!n)return 1;
	if(n->left){
		if(n->left->parent != n)return 0;
		if(!cr8r_avl_check_links(n->left))return 0;
	}
	if(n->right){
		if(n->right->parent != n)return 0;
		if(!cr8r_avl_check_links(n->right))return 0;
	}
	return 1;
}

inline static int cr8r_avl_check_balance(cr8r_avl_node *n){
	if(!n)return 0;
	int l, r;
	if((l = cr8r_avl_check_balance(n->left)) == -1)return -1;
	if((r = cr8r_avl_check_balance(n->right)) == -1)return -1;
	if(r - l == n->balance)return 1 + (l > r ? l : r);
	return -1;
}

#ifdef DEBUG
#include <assert.h>
#define CR8R_AVL_ASSERT_LINKS(n) assert(cr8r_avl_check_links(n))
#define CR8R_AVL_ASSERT_BALANCE(n) assert(cr8r_avl_check_balance(n) != -1)
#define CR8R_AVL_ASSERT_ALL(n) do{CR8R_AVL_ASSERT_LINKS(n);CR8R_AVL_ASSERT_BALANCE(n);}while(0)
#else
#define CR8R_AVL_ASSERT_LINKS(n)
#define CR8R_AVL_ASSERT_BALANCE(n)
#define CR8R_AVL_ASSERT_ALL(n)
#endif

