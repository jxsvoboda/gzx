/*
 * String utility functions
 */

#include <ctype.h>
#include <string.h>
#include "strutil.h"

/*
 * case-insensitive strcmp
 */
int strcmpci(char *a, char *b) {
  while(*a && *b && tolower(*a)==tolower(*b)) {
    a++; b++;
  }
  return tolower(*a) - tolower(*b);
}
