#include <mailutils/argp.h>

#define MH_OPT_BOOL 1
#define MH_OPT_ARG  2

struct mh_option
{
  char *opt;
  int match_len;
  int key;
  int flags;
  char *arg;
};

extern int mh_optind;
extern char *mh_optarg;

int mh_getopt (int argc, char **argv, struct mh_option *mh_opt);
int mh_argp_parse (int argc, char **argv,
		   struct argp_option *option,
		   struct mh_option *mh_option,
		   char *argp_doc, char *doc,
		   int (*handler)(), void *closure);
void mh_help (struct mh_option *mh_option);
void mh_license (const char *name);
