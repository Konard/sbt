#include <stdio.h> //printf,fgets
#include <string.h> // strdup
#include <stdlib.h> // free
#include <avl.h>

#define BUFSIZE 8192

int main(void) {
	char buf[]= "123";
	avl_tree_t *tree;
	avl_node_t *node;
	tree = avl_alloc_tree((avl_compare_t)strcmp, (avl_freeitem_t)free);
#define N (10*1000*1000)
//#define N 1000000
	for(int i = 0; i < N; i++)
		avl_insert(tree, strdup(buf));
//	for(node = tree->head; node; node = node->next)
//		printf("%s", node->item);
	avl_free_tree(tree);
}
