typedef struct list_node {
	struct  list_node *next;
	struct  list_node *prev;
	int value;
} list_node_t;

typedef struct list {
	int n; /* the number of nodes */
	list_node_t *head; /* the first node */
	list_node_t *tail; /* the last node */
} list_t;

/* Addition */
void add_front(list_t *list, int value);
void add_rear(list_t *list, int value);
void add_at_index(list_t *list, int value, int index);

/* Deletion */
void remove_front(list_t *list);
void remove_rear(list_t *list);
void remove_at_index(list_t *list, int index);
void clear(list_t *list);

/* Search */
int index_of(list_t *list, int value);
int occurrences(list_t *list, int value);

/* Sort */
void sort_decreasing(list_t *list);
void sort_increasing(list_t *list);
void reverse(list_t *list);
 
/* Misc */
void * emalloc(size_t size);
void printlist(list_t list);
list_t* make_list();