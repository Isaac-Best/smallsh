#ifndef SIGNAL_HANDLING_H
#define SIGNAL_HANDLING_H

#include <signal.h>

void handle_SIGINT(int signo);
void manage_background(void);
void reset_flags(void);

#endif // SIGNAL_HANDLING_H
