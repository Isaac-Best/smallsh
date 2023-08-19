#define _POSIX_C_SOURCE 200809L
#include "flags.h"


char *words[MAX_WORDS] = {0};
size_t wordct = 0;

int input_flag = 0;
int output_flag = 0;
int append_flag = 0;
char *file_in_descr = NULL;
char *file_out_descr = NULL;
int background_flag = 0;

// shell environment variables 
pid_t shell_pid;  // $$ --> set in main b4 loop 
int last_exit_status_FG = 0; // &? --> set in execute parent branch 
int last_BG_process_pid = 0; // $! --> set in execute parent branch  
int interactive_mode = 1; // default interactive mode 

struct sigaction original_SIGINT_action = {0};
struct sigaction original_SIGTSTP_action = {0};