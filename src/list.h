
#ifndef LIST_H
#define LIST_H

// Node of the list
struct list_node_t {
    // Pointer to the next node in the list
    struct list_node_t *next;

    // Payload
    void *data;
};

// List descriptor
struct list_t {
    // Pointer to first node in the list
    struct list_node_t *first;

    // Pointer to last node in the list
    struct list_node_t *last;
};

// Type to iterate over the list
struct list_iterator_t {
    // The list where the iterator points to
    struct list_t *l;

    // The current node
    struct list_node_t *n;
};

/* Initialize the list.

   Parameters:
     * l: the list.

   Return 0 if the list was correctly initialized, 1 otherwise.
*/
int
list_init(struct list_t *l);

/* Add an element at the end of the list.

   Parameters:
     * l: the list.
     * elem: the element to add.

   Return 0 if the element was correctly added, -1 otherwise. In case of error,
   errno is set accordingly.
*/
int
list_add(struct list_t *l, void *elem);

/* Return an iterator to iterate over the elements in the list.

   Parameters:
     * l: the list to iterate.
     * i: output parameter that contains the iterator pointing before the first
         element.
*/
void
list_iterator_begin(struct list_t *d, struct list_iterator_t *i);

/* Return the next element pointed by the iterator i. Increments the position
   of the iterator.

   Parameters:
     * i: the iterator. At output, the iterator points to the next element.
     * elem: output parameter that contains the current element's value.

   Return 1 if there's next element, 0 if there's no more elements.
*/
int
list_iterator_next(struct list_iterator_t *i, void **elem);

#endif

