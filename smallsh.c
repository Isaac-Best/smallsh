#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#ifndef MAX_WORDS
  #define MAX_WORDS 512
#endif

char *words[MAX_WORDS] = {0};
size_t wordct ;

size_t wordsplit(char const *line);
char * expand(char const *word);

// shell enviroment variables 
pid_t shell_pid;  // $$ --> set in main b4 loop 
int last_exit_status_FG; // &? --> set in execute parent branch 
int last_BG_process_pid; // $! --> set in execute parent branch  

int interactive_mode = 1; // default interactive mode 

bool check_builtin(char **words); // my check built in function 
void execute(char **cmd_args); // execute function 

char **tokenize(char **words, size_t nwords); // my tokenize function 
int input_flag = 0;
int output_flag = 0;
int append_flag = 0; 
char *file_in_descr = NULL;
char *file_out_descr = NULL; 
int background_flag = 0;

// ctrl - C 
void handle_SIGINT(int signo) {
  // do nothing
}


struct sigaction original_SIGINT_action = {0}, original_SIGTSTP_action = {0}; 

void manage_background(void); 
void reset_flags(void); 

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
