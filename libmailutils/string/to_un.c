#ifndef STR_TO_FUN
# error "STR_TO_FUN not defined"
#endif
#ifndef STR_TO_TYPE
# error "STR_TO_TYPE not defined"
#endif
#ifndef STR_TO_MAX
# error "STR_TO_MAX not defined"
#endif

static int
STR_TO_FUN (void *tgt, char const *string, char **errmsg)
{
  STR_TO_TYPE *ptr = tgt;
  uintmax_t v;
  char *p;
  
  errno = 0;
  v = strtoumax (string, &p, 10);
  if (errno)
    return errno;
  if (*p)
    return EINVAL;
  if (v <= STR_TO_MAX)
    {
      *ptr = (STR_TO_TYPE) v;
      return 0;
    }
  return ERANGE;
}

#undef STR_TO_FUN
#undef STR_TO_TYPE
#undef STR_TO_MAX
