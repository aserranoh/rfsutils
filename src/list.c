
/* list.c
   Implements a linked list.
   
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

#include "list.h"

#include <stdlib.h> // malloc

/* Initialize the list.

   Parameters:
     * l: the list.

   Return 0 if the list was correctly initialized, 1 otherwise.
*/
int
list_init(struct list_t *l)
{
    struct list_node_t *sentinel;

    sentinel = (struct list_node_t*)malloc(sizeof(struct list_node_t));
    if (!sentinel) {
        return 1;
    }
    sentinel->next = NULL;
    l->first = sentinel;
    l->last = sentinel;
    return 0;
}

/* Add an element at the end of the list.

   Parameters:
     * l: the list.
     * elem: the element to add.

   Return 0 if the element was correctly added, -1 otherwise. In case of error,
   errno is set accordingly.
*/
int
list_add(struct list_t *l, void *elem)
{
    struct list_node_t *n;

    // Allocate the new node
    n = (struct list_node_t*)malloc(sizeof(struct list_node_t));
    if (!n) {
        return 1;
    }
    // Initialize the new node
    n->next = NULL;
    n->data = elem;
    l->last->next = n;
    l->last = n;
    return 0;
}

/* Return an iterator to iterate over the elements in the list.

   Parameters:
     * l: the list to iterate.
     * i: output parameter that contains the iterator pointing before the first
         element.
*/
void
list_iterator_begin(struct list_t *l, struct list_iterator_t *i)
{
    i->l = l;
    i->n = l->first;
}

/* Return the next element pointed by the iterator i. Increments the position
   of the iterator.

   Parameters:
     * i: the iterator. At output, the iterator points to the next element.
     * elem: output parameter that contains the current element's value.

   Return 1 if there's next element, 0 if there's no more elements.
*/
int
list_iterator_next(struct list_iterator_t *i, void **elem)
{
    if (!i->n->next) {
        return 0;
    } else {
        i->n = i->n->next;
        *elem = i->n->data;
        return 1;
    }
}

