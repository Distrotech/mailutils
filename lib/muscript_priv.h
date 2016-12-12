struct mu_script_fun
{
  char *lang;
  char *suf;
  int (*script_init) (const char *, const char **, mu_script_descr_t *);
  int (*script_done) (mu_script_descr_t);
  int (*script_process) (mu_script_descr_t, mu_message_t);
  int (*script_log_enable) (mu_script_descr_t descr, const char *name,
			    const char *hdr);
};

extern struct mu_script_fun mu_script_python;
extern struct mu_script_fun mu_script_sieve;
extern struct mu_script_fun mu_script_scheme;
