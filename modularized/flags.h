#ifndef FLAGS_H
#define FLAGS_H


#include <stddef.h>
#include <sys/types.h>
#include <signal.h>


#define MAX_WORDS 512

extern char *words[MAX_WORDS];
extern size_t wordct;

extern int input_flag;
extern int output_flag;
extern int append_flag;
extern char *file_in_descr;
extern char *file_out_descr;
extern int background_flag;

// Shell environment variables
extern pid_t shell_pid; 
extern int last_exit_status_FG; 
extern int last_BG_process_pid;
extern int interactive_mode;


extern struct sigaction original_SIGINT_action;
extern struct sigaction original_SIGTSTP_action;

#endif // FLAGS_H
