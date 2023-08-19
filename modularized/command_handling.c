#define _POSIX_C_SOURCE 200809L

#include "command_handling.h"
#include "flags.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>




// set global variables for input flags < > >> 
// set descripto file in and outs 
// handle & background flag 
// dynamically allocate a new string for execvp  
char** tokenize(char **words, size_t nwords){
  char **cmd_args = malloc((nwords + 1) * sizeof(char*)); 
  int cmd_arg_counter = 0;
  
  if (!cmd_args) {
    err(1, "tokenize malloc\n");
  }
  
  for (int i = 0; i < nwords; i++){
    char *current_word = words[i];
    
    if (strcmp(current_word, "<") == 0) {
      input_flag = 1;
      if (++i < nwords) {
        file_in_descr = words[i];
      } else {
        fprintf(stderr, "Missing argument for input redirection\n");
        free(cmd_args);
        return NULL;
      }
    } else if (strcmp(current_word, ">") == 0) {
      output_flag = 1;
      append_flag = 0;

      if (++i < nwords) {
        file_out_descr = words[i];
        
        // multiple redirect operators @_@ mwhahaha
        int fd = open(words[i], O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (fd == -1) perror("open"); 
        close(fd); 

      } else {
        fprintf(stderr, "Missing argument for output redirection\n");
        free(cmd_args);
        return NULL;
      }
    } else if (strcmp(current_word, ">>") == 0) {
      append_flag = 1;
      output_flag = 0; 
      if (++i < nwords) {
        file_out_descr = words[i];
      } else {
        fprintf(stderr, "Missing argument for append redirection\n");
        free(cmd_args);
        return NULL;
      }
    } else if (strcmp(current_word, "&") == 0) {
      if (i == nwords - 1) {
        background_flag = 1;
      } else {
        fprintf(stderr, "Invalid background request\n");
        free(cmd_args);
        return NULL;
      }
    } else {
      cmd_args[cmd_arg_counter++] = current_word;
    }
  }

  cmd_args[cmd_arg_counter] = NULL;
  cmd_args = realloc(cmd_args, (cmd_arg_counter + 1) * sizeof(char*));
  if (!cmd_args)
    err(1, "realloc 3");
  return cmd_args;
}

// should take exit status of the last FG command 
// error if more than one arg 
// default behavior is to exit with last FG exit status 
bool check_builtin(char **words){
  if ( !wordct ) return true ; // blank line is a nothing builtin

  if (strcmp(words[0], "cd") == 0){
    if (wordct == 2) {
      if (chdir(words[1]) != 0) fprintf(stderr, "chdir not valid");
    }
    else {
          if (chdir(getenv("HOME")) != 0) fprintf(stderr, "chdir not valid HOME");
    }
    return true; // need to make sure the rest of the program does not run after a cd 
  }
  else if (strcmp(words[0], "exit") == 0){
      if (words[1] != NULL ){
        char *end_ptr;
        long is_int = strtol(words[1], &end_ptr, 10);

        if (*end_ptr != '\0') { // will be an int if the entire thing was successfuly parsed 
          fprintf(stderr, "More than one arg that is not an int");
          return true; 
        }
        // the scenario where it is a valid int  
        exit((int)is_int);
      }
      // default exit 
      exit(last_exit_status_FG);
  }

  return false; 
} 

// fork child process 
// if its a FG preform a wait action 
// if its a BG continue like normal add the SPAWN PID to the BG array 
void execute(char **cmd_args){
    pid_t spawnPid = fork();
    int childStatus; 

	switch(spawnPid){
	case -1:
		perror("fork()\n");
		exit(1);
		break;
	case 0: // In the child process 
      sigaction(SIGINT, &original_SIGINT_action, NULL);
      sigaction(SIGTSTP, &original_SIGTSTP_action, NULL); 

      if (input_flag) { 
        if ( ! ( stdin = freopen( file_in_descr, "r", stdin ))){
          err( 1, "Redirect input to '%s'", file_in_descr );
        }
      }

      if ( output_flag || append_flag ){
        if ( ! ( stdout = freopen( file_out_descr, append_flag ? "a" : "w", stdout ))){
          err( 1, "Redirect output to '%s'", file_out_descr );
        }
      }

      execvp(cmd_args[0], cmd_args );
      err( 1, "execvp '%s' failed", cmd_args[0]);
      exit(1);
      break;
	default: // In the parent process
      free( cmd_args );
        
      if (!background_flag) { // FG process, wait for child to complete / monitor child process here 
        spawnPid = waitpid(spawnPid, &childStatus, WUNTRACED | WCONTINUED);
        if (spawnPid == -1){
          perror("waitpid");
        }
        // if child process stopped sent sigcont print message to stderr update its status to a BG process + stop waiting 
        if (WIFSTOPPED(childStatus)) {
          if (kill(spawnPid, SIGCONT) < 0)
            fprintf(stderr, "error signaling child process"); 
          
          fprintf(stderr, "Child process %d stopped. Continuing.\n", spawnPid);
          last_BG_process_pid = spawnPid; // update $! 
          return ; // stop waiting
        } else if (WIFSIGNALED(childStatus)) {
          last_exit_status_FG = 128 + WTERMSIG(childStatus);
        } else if (WIFEXITED(childStatus)) { // exit normally 
          last_exit_status_FG = WEXITSTATUS(childStatus);
        }
      } else { // background process 
        last_BG_process_pid = spawnPid;  
        background_flag = 0; 
     }
        
     break; 
  }
} // end execute() 