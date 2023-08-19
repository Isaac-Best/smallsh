#ifndef COMMAND_HANDLING_H
#define COMMAND_HANDLING_H

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stddef.h>
#include <stdbool.h>





void execute(char **cmd_args);
bool check_builtin(char **words);
char** tokenize(char **words, size_t nwords);


#endif // COMMAND_HANDLING_H
