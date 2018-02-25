
#include "readline.h"

#include <stdlib.h> // malloc
#include <string.h> // strlen

// Initial size of the buffer to read lines from the configuration file
#define READLINE_INIT_SIZE  64

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
readline(struct line_t *line, FILE *f)
{
    size_t cur = 0;

    // Allocate space to the line if necessary
    if (!line->size) {
        line->line = malloc(READLINE_INIT_SIZE);
        if (!line->line) {
            return -1;
        }
        line->size = READLINE_INIT_SIZE;
    }
    // Read the next line
    while (fgets(line->line + cur, line->size - cur, f)) {
        cur += strlen(line->line + cur);
        if (cur == line->size - 1 && line->line[cur - 1] != '\n') {
            // Incomplete line, resize the buffer and stay in the loop
            line->size *= 2;
            line->line = realloc(line->line, line->size);
            if (!line->line) {
                return -1;
            }
        } else {
            // Line complete
            return 1;
        }
    }
    // Here, an error or EOF has happened
    if (ferror(f)) {
        return -1;
    }
    return 0;
}

/* Frees the instance of a line.

   Parameters:
     * line: the line to free.
*/
void
readline_free(struct line_t *line)
{
    free(line->line);
    line->line = NULL;
    line->size = 0;
}

