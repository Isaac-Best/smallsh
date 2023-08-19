#define _POSIX_C_SOURCE 200809L

#include "signal_handling.h"
#include "flags.h"
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <inttypes.h>


// ctrl - C 
void handle_SIGINT(int signo) {
  // do nothing
}

void manage_background(void) {
  int childStatus; 
  pid_t childpid ;

  while ( 0 < (childpid = waitpid(0, &childStatus, WNOHANG | WUNTRACED))){
    if (WIFEXITED(childStatus)){
          fprintf(stderr, "Child process %jd done. Exit status %d.\n", (intmax_t)childpid, WEXITSTATUS(childStatus));
    } else if (WIFSIGNALED(childStatus)) {
          fprintf(stderr, "Child process %jd done. Signaled %d.\n", (intmax_t)childpid, WTERMSIG(childStatus));
    } else if (WIFSTOPPED(childStatus)) {
          kill(childpid, SIGCONT);
          fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t)childpid);
    }
  }
}

void reset_flags(void) {
  input_flag = 0;
  output_flag = 0;
  append_flag = 0; 
}
