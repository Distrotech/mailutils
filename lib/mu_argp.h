#include <mailutils/mailbox.h>
#include <argp.h>

#define MODE_INTERACTIVE 0
#define MODE_DAEMON 1

struct daemon_param {
  int mode;
  size_t maxchildren;
  unsigned int port;
  unsigned int timeout;
};

#ifdef USE_LIBPAM
extern char *pam_service;
#endif
extern int log_facility;
extern int mu_argp_error_code;

extern void mu_create_argcv __P((const char *capa[],
				 int argc, char **argv,
				 int *p_argc, char ***p_argv));
extern error_t mu_argp_parse __P((const struct argp *argp, 
				  int *p_argc, char ***p_argv,  
				  unsigned flags,
				  const char *capa[],
				  int *arg_index,     
				  void *input));

