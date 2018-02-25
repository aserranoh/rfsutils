
#ifndef READLINE_H
#define READLINE_H

#include <stdio.h>
#include <sys/types.h>

// Type that represents a line by a dynamic array.
struct line_t {
    // The buffer to store the line
    char *line;

    // The current max length of the line
    size_t size;
};

// Define to initialize a static instance of struct line_t
#define LINE_INIT   {0, 0}

/* Reads a line of a file into a dynamic array.
   If the next line is too long for the array to store it, it is reallocated
   to a suitable size.

   Parameters:
     * line: a member of struct line_t that contains the line and attributes
         to manage its size.
     * f: the file where the lines are read from.

   Return:
     * 1 if a line is read.
     * 0 if EOF.
     * -1 if error.
*/
int
readline(struct line_t *line, FILE *f);

/* Frees the instance of a line.

   Parameters:
     * line: the line to free.
*/
void
readline_free(struct line_t *line);

#endif

