
/* readline.h
   Read a line from a file stream into a dynamic array.
   
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

