#include <argp.h>

#define MODE_INTERACTIVE 0
#define MODE_DAEMON 1

struct daemon_param {
  int mode;
  size_t maxchildren;
  unsigned int port;
  unsigned int timeout;
};


extern char *maildir;
extern int log_facility;
extern struct argp_child mu_common_argp_child[];
extern struct argp_child mu_daemon_argp_child[];

extern void mu_create_argcv (int argc, char **argv,
			     int *p_argc, char ***p_argv);

