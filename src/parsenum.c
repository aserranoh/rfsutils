
/* parse.c
   Function to parse a number from a string.
   
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

#include <limits.h> // UINT_MAX

/* Function that parses some digits and creates a number.
   It doesn't accept minus sign or other funcy stuff.

   Parameters:
     * s: string with the text.
     * eptr: at exit, points to the next character to the last digit.
     * i: at exit, contains the result.
*/
int
parse_number(const char *s, const char **eptr, unsigned int *i)
{
    const char *ptr = s;
    unsigned long long int t = 0;

    // Add the numbers to the total while there's numbers in the string
    while (*ptr >= '0' && *ptr <= '9') {
        t = t * 10 + (*ptr - '0');
        // Check that the result fits in an unsigned int
        if (t > UINT_MAX) {
            *i = 0;
            return -1;
        }
        ptr++;
    }
    // Return the next character to the last digit and the result
    *eptr = ptr;
    *i = t;
    return 0;
}

