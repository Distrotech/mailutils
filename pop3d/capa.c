/* GNU POP3 - a small, fast, and efficient POP3 daemon

 * Code for capa support by Sean 'Shaleh' Perry <shaleh@debian.org>
 * added 4/2/1999
 *
 */
#include "pop3d.h"

int
pop3_capa (const char *arg)
{
  if (strlen (arg) != 0)
    return ERR_BAD_ARGS;

  if (state != AUTHORIZATION && state != TRANSACTION)
    return ERR_WRONG_STATE;

  fprintf (ofile, "+OK Capability list follows\r\n");
  fprintf (ofile, "TOP\r\n");
  fprintf (ofile, "USER\r\n");
  fprintf (ofile, "RESP-CODES\r\n");
  if (state == TRANSACTION)	/* let's not advertise to just anyone */
    fprintf (ofile, "IMPLEMENTATION %s %s\r\n", IMPL, VERSION);
  fprintf (ofile, ".\r\n");
  return OK;
}
