#ifndef STRING_PROCESSING_H
#define STRING_PROCESSING_H

#include <stddef.h>

size_t wordsplit(char const *line);
char param_scan(char const *word, char const **start, char const **end);
char * build_str(char const *start, char const *end);
char * expand(char const *word);

#endif // STRING_PROCESSING_H
