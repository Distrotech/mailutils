/* GNU copyright notice */

#include "pop3d.h"

void
pop3_sigchld (int signal)
{
  pid_t pid;
  int stat;

  while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
      --children;
  return;
}
