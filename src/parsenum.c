
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

