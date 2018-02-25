
/* Function that parses some digits and creates a number.
   It doesn't accept minus sign or other funcy stuff.

   Parameters:
     * s: string with the text.
     * eptr: at exit, points to the next character to the last digit.
     * i: at exit, contains the result.

   Return 0 if the number was correctly parsed, -1 if the number is out of the
   range of an unsigned int.
*/
int
parse_number(const char *s, const char **eptr, unsigned int *i);

