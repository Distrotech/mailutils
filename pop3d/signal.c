/* GNU copyright notice */

#include "pop3d.h"

void
pop3_sigchld (int signo)
{
  pid_t pid;
  int status;

  (void)signo;
  while ( (pid = waitpid(-1, &status, WNOHANG)) > 0)
      --children;
}
