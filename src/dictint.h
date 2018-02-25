
#ifndef DICTINT_H
#define DICTINT_H

#include "list.h"

#include <sys/types.h>  // size_t

struct dictint_node_t {
    // The next element
    struct dictint_node_t *next;

    // The key
    int key;

    // The value
    void *value;
};

struct bucket_t {
    // The first element in this bucket
    struct dictint_node_t *first;

    // The last element in this bucket
    struct dictint_node_t *last;
};

// Dictionary of int keys
struct dictint_t {
    // A dynamic array of lists
    struct bucket_t *table;

    // Size of the array
    size_t table_size;

    // Number of elements in the dictionary
    size_t numelems;
};

// Type used to iterate over the elements of the dictionary
struct dictint_iterator_t {
    // Dictionary being iterated
    struct dictint_t *d;

    // Current bucket
    int bucket;

    // Current node
    struct dictint_node_t *node;
};

/* Initialize the dictionary.

   Parameters:
     * d: the dictionary to initialize.

   Return 0 if the initialization was correct, 1 otherwise, and errno is set
   accordingly.
*/
int
dictint_init(struct dictint_t *d);

/* Adds an element to the dictionary.

   Parameters:
     * d: the dictionary.
     * key: the element's key.
     * elem: the element to add.

   Return 0 if the element is correctly added, -1 otherwise. In the latter
   case, errno is set accordingly.
*/
int
dictint_add(struct dictint_t *d, int key, void *elem);

/* Return an element of the dictionary given its key, or NULL if the element is
   not in the dictionary.

   Parameters:
     * d: the dictionary.
     * key: the key.
*/
void *
dictint_get(struct dictint_t *d, int key);

/* Return an iterator to iterate over the elements in the dictionary.

   Parameters:
     * d: the dictionary to iterate.
     * i: output parameter that contains the iterator pointing to the first
         element.
*/
void
dictint_iterator_begin(struct dictint_t *d, struct dictint_iterator_t *i);

/* Return the next element pointed by the iterator i. Increments the position
   of the iterator.

   Parameters:
     * i: the iterator. At output, the iterator points to the next element.
     * key: output parameter that contains the current element's key.
     * value: output parameter that contains the current element's value.

   Return 1 if there's next element, 0 if there's no more elements.
*/
int
dictint_iterator_next(struct dictint_iterator_t *i, int *key, void **value);

/* Return the number of elements in the dictionary.

   Parameters:
     * d: the dictionary.
*/
size_t
dictint_size(struct dictint_t *d);

#endif

