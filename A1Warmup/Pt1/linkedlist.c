/*
 * A more idomatic implementation of a doubly linked list (vs the one in "CSC 360\C Review")
 */

#include <stdlib.h>
#include <stdio.h>
#include "linkedlist.h"
#include <assert.h>

int main() {
	#ifdef DEBUG
		printf("=====================\n");
		printf("Running in DEBUG mode\n");
		printf("=====================\n");
	#endif

	// Addition and deletion	
	list_t *list = make_list();
	printlist(*list);
	add_front(list, 1);
	printlist(*list);
	add_rear(list, 2);
	printlist(*list); 
	add_rear(list, 3);
	printlist(*list);
	add_at_index(list, 100, 0);
	printlist(*list);
	add_at_index(list, 200, list->n);
	printlist(*list);
	add_at_index(list, 300, 2);
	printlist(*list);
	remove_rear(list);
	printlist(*list);
	remove_front(list);
	printlist(*list);
	remove_at_index(list, 1); // remove the 300
	printlist(*list);

	// index_of
	list_node_t *p;
	for(p = list->head; p != NULL; p = p->next) {
		printf("%d ", index_of(list, p->value));
	}
	printf("%d\n", index_of(list, 1010101010));

	// Reverse
	reverse(list);
	printlist(*list);

	// Sort
	int i;
	for(i = 100; i < 104; i++) {
		add_front(list, i);
	}
	sort_decreasing(list);
	printlist(*list);
	sort_increasing(list);
	printlist(*list);

	// Valgrind
	clear(list);
	free(list);

	#ifdef DEBUG
		printf("Flushing:\n");
		printf("---------\n");
		fflush(stdout);
	#endif
}

/*
 * Helper function to create (and allocate space for) new nodes
 */
static list_node_t* make_node(int value, list_node_t * prev, list_node_t * next) {
	#ifdef DEBUG
		printf("Creating new node with value %d\n", value);
	#endif	
	list_node_t * new_node = (list_node_t *) emalloc(sizeof(list_node_t));
	new_node -> value = value;
	new_node -> prev = prev;
	new_node -> next = next;
	return new_node;
}

/*
 * Adds the value to the front of the list. 
 */
void add_front(list_t *list, int value) {
	assert(list != NULL);
	#ifdef DEBUG
		printf("Adding %d to front of list\n", value);
	#endif
	list_node_t *new_node = make_node(value, NULL, list->head);
	if(list->n == 0) {
		list->tail = new_node;
	} else {
		list->head->prev = new_node;
	}
	list->head = new_node;
	list->n = list->n + 1;
}

/*
 * Add the value to the end of the list
 */
void add_rear(list_t *list, int value) {
	assert(list != NULL);
	#ifdef DEBUG
		printf("Adding %d to end of list\n", value);
	#endif
	list_node_t *new_node = make_node(value, list->tail, NULL);
	if(list->n == 0) {
		list->head = new_node;
	} else {
		list->tail->next = new_node;
	}
	list->tail = new_node;
	list->n = list->n + 1;
}

/*
 * Add the value at the indexth position in the list (starting at 0)
 * 0 <= index <= list->n
 */
void add_at_index(list_t *list, int value, int index) {
	assert(list != NULL);
	assert(0 <= index && index <= list->n);
	#ifdef DEBUG
		printf("Adding %d to list at index %d\n", value, index);
	#endif
	if(index == 0) {
		add_front(list, value);
	} else if(index == list->n) {
		add_rear(list, value);
	} else {
		list_node_t *p = list->head;
		int i;
		for(i = 0; i < index-1; i++) {
			p = p->next;
		}
		/* p now points to the node at position index-1 */
		list_node_t *new_node = make_node(value, p, p->next);
		p->next = new_node;
		new_node->next->prev = new_node;
		list->n = list->n + 1;
	}
}

/*
 * Malloc wrapper that checks for allocation error
 */
void * emalloc(size_t size) {
	assert(size > 0);
	void * temp = malloc(size);
	if(temp == NULL) {
		printf("ERROR. malloc failed. exiting.\n");
		exit(0);
	}
	return temp;
}

/*
 * Removes first element from a non-empty list
 */
void remove_front(list_t *list) {
	assert(list != NULL);
	assert(list->n > 0);
	#ifdef DEBUG
		printf("removing first element\n");
	#endif
	list_node_t *old_node = list->head;
	list->head = list->head->next;
	list->n = list->n - 1;
	if(list->n > 0) {
		list->head->prev = NULL;
	} else {
		list->tail = NULL;
	}
	free(old_node);
}

/*
 * Removes the last element from a non-empty list
 */
void remove_rear(list_t *list) {
	assert(list != NULL);
	assert(list->n > 0);
	#ifdef DEBUG
		printf("removing last element\n");
	#endif
	list_node_t *old_node = list->tail;
	list->tail = old_node->prev;
	list->n = list->n - 1;
	if(list->n > 0) {
		list->tail->next = NULL;
	} else {
		list->head = NULL;
	}
	free(old_node);
}

/*
 * Removes the specified element from a non-empty list
 */
void remove_at_index(list_t *list, int index) {
	assert(list != NULL);
	assert(0 <= index && index < list->n);
	#ifdef DEBUG
		printf("removing element at index %d\n", index);
	#endif
	if(index == 0) {
		remove_front(list);
	} else if(index == list->n - 1) {
		remove_rear(list);
	} else {
		list_node_t *p = list->head;
		int i;
		for(i = 0; i < index - 1; i++) {
			p = p->next;
		}
		list_node_t *old_node = p->next;
		p->next = old_node->next;
		p->next->prev = p;
		list->n = list->n - 1;
		free(old_node);
	}
}

/*
 * Removes all items from the list
 */
void clear(list_t *list) {
	assert(list != NULL);
	list_node_t *p = list->head;
	list_node_t *temp;
	while(p != NULL) {
		temp = p;
		p = p->next;
		free(temp);
	}
}

/*
 * Returns the index of the first occurance of the value. 
 * Returns -1 if the value does not occur. 
 */
int index_of(list_t *list, int value) {
	list_node_t *p;
	int index = 0;
	for(p = list->head; p; p = p->next) {
		if(p->value == value) {
			return index; 
		}
		index++;
	}
	return -1;
}

/*
 * Returns the number of times the value appears in the list 
 */
int occurrences(list_t *list, int value) {
	list_node_t *p;
	int count = 0;
	for(p = list->head; p; p = p->next) {
		if(p->value == value) {
			count++; 
		}
	}
	return count;
}

/*
 * Quick sort
 */
void sort_decreasing(list_t *list) {
	assert(list != NULL);
	#ifdef DEBUG
		printf("sorting list in decreasing order\n");
	#endif
	sort_increasing(list);
	reverse(list);
}

/*
 * Quick sort
 *
 * Implementation isn't the greatest :( 
 * 		There's a bunch of space overhead
 * 		Any external node pointers are destroyed
 * But hey its simple :) 
 */
void sort_increasing(list_t *list) {
	assert(list != NULL);
	#ifdef DEBUG
		printf("sorting list in increasing order\n");
	#endif
	if(list->n <= 1) {
		return;
	} else {
		int pivot = list->head->value;
		list_t *lower = make_list();
		list_t *equal = make_list();
		list_t *greater = make_list();
		list_node_t *p;
		for(p = list->head; p != NULL; p = p->next) {
			if(p->value < pivot) {
				add_rear(lower, p->value);
			} else if(p->value == pivot) {
				add_rear(equal, p->value);
			} else {
				add_rear(greater, p->value);
			}
		}
		sort_increasing(lower);
		sort_increasing(greater);
		clear(list);
		if(lower->n > 0) {
			list->head = lower->head; 
			lower->tail->next = equal->head;
		} else {
			list->head = equal->head;
		}
		equal->head->prev = lower->tail; 
		equal->tail->next = greater->head; 
		if(greater->n > 0) {
			list->tail = greater->tail;
			greater->head->prev = equal->tail;
		} else {
			list->tail = equal->tail;
		}
		list->n = lower->n + equal->n + greater->n;
		free(lower);
		free(equal);
		free(greater);
	}
}


void reverse(list_t *list) {
	assert(list != NULL);
	#ifdef DEBUG
		printf("reversing list\n");
	#endif

	if(list->n <= 1) {
		return;
	} else{
		/*
		 * Swap next/prev pointers for each node
		 * Swap head/tail pointers for the list
		 */

		list_node_t *p;
		list_node_t *temp;
		for(p = list->head; p != NULL; p = p->prev) {
			temp = p->next;
			p->next = p->prev;
			p->prev = temp;
		}
		temp = list->head;
		list->head = list->tail;
		list->tail = temp;
	}
}

list_t* make_list() {
	list_t *list = emalloc(sizeof(list_t));
	list->n = 0;
	list->head = NULL;
	list->tail = NULL;
	return list;
}

void printlist(list_t list) {
	#ifdef DEBUG
		printf("------------------------------------\n");
		printf("printlist called on list of size %d\n", list.n);
	#endif
	list_node_t *p;
	printf("{");
	for(p = list.head; p != NULL; p = p->next) {
		printf("%d", p->value);
		if(p->next != NULL) {
			printf(", ");
		}
	}
	printf("}\n");
}