#include <config.h>
#include <mailutils/mailutils.h>
#include <assert.h>

static int
print_param (void *item, void *data)
{
  size_t *n = data;
  struct mu_param *p = item;
  printf ("%2zu: %s=%s\n", *n, p->name, p->value);
  ++*n;
  return 0;
}

int
parse (char const *input)
{
  mu_content_type_t ct;
  int rc;
    
  rc = mu_content_type_parse (input,  &ct);
  if (rc)
    {
      mu_error ("%s", mu_strerror (rc));
      return 1;
    }

  printf ("type = %s\n", ct->type);
  printf ("subtype = %s\n", ct->subtype);
  if (ct->trailer)
    printf ("trailer = %s\n", ct->trailer);
  if (!mu_list_is_empty (ct->param))
    {
      size_t n = 0;
      mu_list_foreach (ct->param, print_param, &n);
    }
  mu_content_type_destroy (&ct);
  return 0;
}

int
main (int argc, char **argv)
{
  char *buf = NULL;
  size_t size = 0, n;
  int rc;
  
  mu_set_program_name (argv[0]);
  mu_stdstream_setup (MU_STDSTREAM_RESET_NONE);

  if (argc == 2)
    return parse (argv[1]);
  while ((rc = mu_stream_getline (mu_strin, &buf, &size, &n)) == 0 && n > 0)
    {
      mu_rtrim_class (buf, MU_CTYPE_ENDLN);
      if (parse (buf))
	rc = 1;
    }
  return rc;
}
  
