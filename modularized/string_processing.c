#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "string_processing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <ctype.h>
#include "flags.h"


/* Splits a string into words delimited by whitespace. Recognizes
 * comments as '#' at the beginning of a word, and backslash escapes.
 *
 * Returns number of words parsed, and updates the words[] array
 * with pointers to the words, each as an allocated string.
 */
size_t wordsplit(char const *line) {
  size_t wlen = 0;
  size_t wind = 0;
  char const *c = line;
  
  for (;*c && isspace(*c); ++c); /* discard leading space */

  for (; *c;) { // iterate until NULL character 
    if (wind == MAX_WORDS) break;
    /* read a word */
    if (*c == '#') break;
    for (;*c && !isspace(*c); ++c) {
      if (*c == '\\') ++c;
      
      void *tmp = realloc(words[wind], sizeof **words * (wlen + 2));
      
      if (!tmp) err(1, "realloc 1");
      
      words[wind] = tmp;
      words[wind][wlen++] = *c; 
      words[wind][wlen] = '\0';
    }
    ++wind;
    wlen = 0;
    
    for (;*c && isspace(*c); ++c);
  }
  
  return wind;
} // end wordsplit()



/* Find next instance of a parameter within a word. Sets
 * start and end pointers to the start and end of the parameter
 * token.
 */

// returns the parameter if found $ ! ? { start and end are set to the parameter indexes
// if no parameter is found return 0 and start and end are set to NULL
char param_scan(char const *word, char const **start, char const **end)
{
  static char const *prev;
  if (!word) word = prev;
  
  char ret = 0;
  *start = 0;
  *end = 0;
  for (char const *s = word; *s && !ret; ++s) {
    s = strchr(s, '$');
    if (!s) break;
    switch (s[1]) {
    case '$':
    case '!':
    case '?':
      ret = s[1];
      *start = s;
      *end = s + 2;
      break;
    case '{':;
      char *e = strchr(s + 2, '}');
      if (e) {
        ret = s[1];
        *start = s;
        *end = e + 1;
      }
      break;
    }
  }
  prev = *end;
  return ret;
}



/* Simple string-builder function. Builds up a base
 * string by appending supplied strings/character ranges
 * to it.
 */

// if start is NULL reset the string, allocates a new one and returns the old 
// if end is NULL it appends the whole start string to the base string 
// otherwise it appends the start - end range to the base string 
char * build_str(char const *start, char const *end)
{
  static size_t base_len = 0;
  static char *base = 0; // retains value accross function calls 

  if (!start) {
    /* Reset; new base string, return old one */
    char *ret = base;
    base = NULL;
    base_len = 0;
    return ret;
  }
  /* Append [start, end) to base string 
   * If end is NULL, append whole start string to base string.
   * Returns a newly allocated string that the caller must free.
   */
  size_t n = end ? end - start : strlen(start);
  size_t newsize = sizeof *base *(base_len + n + 1);
  void *tmp = realloc(base, newsize);
  if (!tmp) err(1, "realloc 2");
  base = tmp;
  memcpy(base + base_len, start, n);
  base_len += n;
  base[base_len] = '\0';

  return base;
}


/* Expands all instances of $! $$ $? and ${param} in a string 
 * Returns a newly allocated string that the caller must free
 */
char *
expand(char const *word)
{

  char buffer1[21]; 
  char buffer2[21]; 
  char buffer3[21];

  if (last_BG_process_pid == 0){
    strcpy(buffer1, "");
   } else {
  sprintf(buffer1, "%d", last_BG_process_pid);
   }

  sprintf(buffer2, "%d", shell_pid);
  sprintf(buffer3, "%d", last_exit_status_FG);

  char const *pos = word;
  const char *start, *end;
  char c = param_scan(pos, &start, &end);
  build_str(NULL, NULL);
  build_str(pos, start);
  while (c) {
    if (c == '!') build_str(buffer1, NULL); 
    else if (c == '$') build_str(buffer2, NULL); 
    else if (c == '?') build_str(buffer3, NULL);
    else if (c == '{') {
      char *param_name = strndup(start + 2, end - start - 3); // excludes ${ }
      char *param_value = getenv(param_name);
      if (!param_value) param_value = "";
      build_str(param_value, NULL);
      free(param_name);
    }
    pos = end;
    c = param_scan(pos, &start, &end);
    build_str(pos, start);
  }
  return build_str(start, NULL);
}
