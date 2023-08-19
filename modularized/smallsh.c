#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>


#include "flags.h"
#include "string_processing.h"
#include "command_handling.h"
#include "signal_handling.h"



int main(int argc, char *argv[])
{
  struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};

  SIGINT_action.sa_handler = SIG_IGN; 
  sigfillset(&SIGINT_action.sa_mask);
  SIGINT_action.sa_flags = 0; 

  SIGTSTP_action.sa_handler = SIG_IGN;
  sigfillset(&SIGTSTP_action.sa_mask); 
  SIGTSTP_action.sa_flags = 0; 

  sigaction(SIGINT, &SIGINT_action, &original_SIGINT_action);
  sigaction(SIGTSTP, &SIGTSTP_action, &original_SIGTSTP_action); 

  FILE *input = stdin;
  char *input_fn = "(stdin)";

  if (argc == 2) {
    input_fn = argv[1];
    input = fopen(input_fn, "re");
    if (!input) err(1, "%s", input_fn);

    // If in non-interactive mode, set signals to default behavior 
    interactive_mode = 0; 
    SIGINT_action.sa_handler = SIG_DFL;
    SIGTSTP_action.sa_handler = SIG_DFL;
    sigaction(SIGINT, &SIGINT_action, NULL);
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
  } else if (argc > 2) {
    errx(1, "too many arguments");
  }

  shell_pid = getpid(); // current process ID 

  char *line = NULL;
  size_t n = 0;
  for (;;) {

    manage_background(); 
    reset_flags(); 

    if (input == stdin) {
      const char *ps1 = getenv("PS1");
      if (!ps1) ps1 = "";

      char *expanded_ps1 = expand(ps1);
      fprintf(stderr, "%s", expanded_ps1);
      free(expanded_ps1);
    }

    if (interactive_mode) { // reading line of input set to function that does nothing
      SIGINT_action.sa_handler = handle_SIGINT; 
      sigaction(SIGINT, &SIGINT_action, NULL);
    }
    
    int result = getline(&line, &n, input);
    if (0 > result) {
      if (feof(input)) exit(last_exit_status_FG); // ctrl - D 
      else if (result == -1 && errno == EINTR) {
        fprintf(stderr, "\n");
        clearerr(input);
        errno = 0; 
        continue; }
      else {err(1, "%s", input_fn);}
    }
    
    if (interactive_mode) { // set it back to signal ignore after reading get line
      SIGINT_action.sa_handler = SIG_IGN;
      sigaction(SIGINT, &SIGINT_action, NULL);
    }
    
    wordct = wordsplit(line);
    
    for (size_t i = 0; i < wordct; ++i) { // expand all words
      char *exp_word = expand(words[i]);
      free(words[i]);
      words[i] = exp_word;
    }

    if (! check_builtin(words)){
      char **cmd_args = tokenize(words, wordct);

      if ( cmd_args )
        execute( cmd_args );
    }
    
    // free stuff b4 next iteration 
    free( line );
    line = NULL;
  }
}



