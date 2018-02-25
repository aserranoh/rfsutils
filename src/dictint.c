
/* dictint.c
   Implementation of a dictonary as an array of linked lists.
   
   Copyright 2018 Antonio Serrano Hernandez

   This file is part of rfsutils.

   rfsutils is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   rfsutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with rfsutils; see the file COPYING.  If not, see
   <http://www.gnu.org/licenses/>.
*/

#include "dictint.h"

#include <stdlib.h> // malloc

// Initial size of the dictionary table. This must be ALLWAYS power of 2.
#define DICTINT_INIT_SIZE   8

// Load factor to trigger a rehash
#define MAX_LOAD_FACTOR     (2.0/3.0)

/* Initialize the dictionary's table.
   The buckets are initialized. Each bucket has a sentinel node.

   Parameters:
     * table: the dictionary's table to be initialized.
     * size: size of the table.

   Return 0 if the table is correctly initialized, 1 otherwise.
*/
static int
dictint_init_table(struct bucket_t *table, size_t size)
{
    struct dictint_node_t *sentinel;

    for (int i = 0; i < size; i++) {
        sentinel = (struct dictint_node_t*)malloc(
            sizeof(struct dictint_node_t));
        if (!sentinel) {
            return 1;
        }
        sentinel->next = NULL;
        table[i].first = sentinel;
        table[i].last = sentinel;
    }
    return 0;
}

/* Initialize the dictionary.

   Parameters:
     * d: the dictionary to initialize.

   Return 0 if the initialization was correct, 1 otherwise, and errno is set
   accordingly.
*/
int
dictint_init(struct dictint_t *d)
{
    // Allocate the dictionary's table
    d->table_size = DICTINT_INIT_SIZE;
    d->table = (struct bucket_t*)malloc(
        sizeof(struct bucket_t) * d->table_size);
    if (!d->table) {
        return 1;
    }
    // Initialize the dictionary's table
    // The bucketsare initialized with a sentinel
    d->numelems = 0;
    if (dictint_init_table(d->table, DICTINT_INIT_SIZE)) {
        return 1;
    }
    return 0;
}

/* Creates a new node with a given pair key/val and inserts it into the given
   bucket of the given table.

   Parameters:
     * table: the table where to add the new node.
     * bucket: the index of the bucket where to add the new node.
     * key: new element's key.
     * val: new element's value.

   Return 0 if the new element is correctly added, 1 otherwise.
*/
static int
dictint_add_node(struct bucket_t *table, int bucket, int key, void *val)
{
    struct dictint_node_t *n;

    // Allocate a new node
    n = (struct dictint_node_t*)malloc(sizeof(struct dictint_node_t));
    if (!n) {
        return 1;
    }
    // Initialize the node
    n->next = NULL;
    n->key = key;
    n->value = val;
    // Insert the node in the corresponding bucket
    table[bucket].last->next = n;
    table[bucket].last = n;
    return 0;
}

/* Resize the table (multiply its size by 2) and rehash its elements.

   Parameters:
     * d: the dictionary.

   Return 0 if the rehashing is correctly done, 1 otherwise.
*/
static int
dictint_rehash(struct dictint_t* d)
{
    struct bucket_t *new_table;
    size_t new_size;
    struct dictint_iterator_t it;
    int key, bucket;
    void *val;

    // Resize is done when load factor (number of elements divided by number
    // of buckets) is greater than MAX_LOAD_FACTOR.
    if (d->numelems/d->table_size > MAX_LOAD_FACTOR) {
        // Alocate the new table
        new_size = d->table_size * 2;
        new_table = (struct bucket_t*)malloc(
            sizeof(struct bucket_t) * new_size);
        if (!new_table) {
            return 1;
        }
        // Initialize the new table
        if (dictint_init_table(new_table, new_size)) {
            return 1;
        }
        // Rehash the table
        dictint_iterator_begin(d, &it);
        while (dictint_iterator_next(&it, &key, &val)) {
            if (!dictint_add_node(new_table, key & (new_size - 1), key, val)) {
                return 1;
            }
        }
        // Update the dictionary with the new table
        free(d->table);
        d->table = new_table;
        d->table_size = new_size;
    }
    return 0;
}

/* Return the node with the given key, that must be in the given bucket.

   Parameters:
     * d: the dictionary.
     * bucket: the bucket index where the node must be searched.
     * key: the key of the node being search.
*/
static struct dictint_node_t*
dictint_get_node(struct dictint_t *d, int bucket, int key)
{
    struct dictint_node_t *n = d->table[bucket].first->next;

    while (n) {
        if (n->key == key) {
            break;
        }
        n = n->next;
    }
    return n;
}

/* Adds an element to the dictionary.

   Parameters:
     * d: the dictionary.
     * key: the element's key.
     * elem: the element to add.

   Return 0 if the element is correctly added, -1 otherwise. In the latter
   case, errno is set accordingly.
*/
int
dictint_add(struct dictint_t *d, int key, void *elem)
{
    struct dictint_node_t *n;
    int bucket;

    // Rehash the table
    if (dictint_rehash(d)) {
        return 1;
    }
    // Search a node with the given key
    bucket = key & (d->table_size - 1);
    n = dictint_get_node(d, bucket, key);
    if (n) {
        // A node with the same key exists, replace the value
        n->value = elem;
    } else {
        // No node with this key exists, create a new one
        if (dictint_add_node(d->table, bucket, key, elem)) {
            return 1;
        }
    }
    d->numelems++;
    return 0;
}

/* Return an element of the dictionary given its key, or NULL if the element is
   not in the dictionary.

   Parameters:
     * d: the dictionary.
     * key: the key.
*/
void*
dictint_get(struct dictint_t *d, int key)
{
    struct dictint_node_t *n;

    n = dictint_get_node(d, key & (d->table_size - 1), key);
    if (n) {
        return n->value;
    }
    return NULL;
}

/* Return an iterator to iterate over the elements in the dictionary.

   Parameters:
     * d: the dictionary to iterate.
     * i: output parameter that contains the iterator pointing before the first
         element.
*/
void
dictint_iterator_begin(struct dictint_t *d, struct dictint_iterator_t *i)
{
    i->d = d;
    i->bucket = 0;
    i->node = d->table[0].first;
}

/* Return the next element pointed by the iterator i. Increments the position
   of the iterator.

   Parameters:
     * i: the iterator. At output, the iterator points to the returned element.
     * key: output parameter that contains the current element's key.
     * value: output parameter that contains the current element's value.

   Return 1 if there's next element, 0 if there's no more elements.
*/
int
dictint_iterator_next(struct dictint_iterator_t *i, int *key, void **value)
{
    // If there's no more buckets, stop iteration
    if (i->bucket >= i->d->table_size) {
        return 0;
    }
    while (1) {
        i->node = i->node->next;
        if (!i->node) {
            // No more nodes in this bucket, go to the next bucket
            i->bucket++;
            if (i->bucket < i->d->table_size) {
                i->node = i->d->table[i->bucket].first;
            } else {
                // No more buckets, stop iteration
                return 0;
            }
        } else {
            // Found next node
            *key = i->node->key;
            *value = i->node->value;
            return 1;
        }
    }
}

/* Return the number of elements in the dictionary.

   Parameters:
     * d: the dictionary.
*/
size_t
dictint_size(struct dictint_t *d)
{
    return d->numelems;
}

