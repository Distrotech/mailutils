/*
 * sieve field cache.
 */

#ifndef SVFIELD_H
#define SVFIELD_H

#define SV_FIELD_CACHE_SIZE 1019

typedef struct sv_field_t
{
  char *name;
  int ncontents;
  char *contents[1];
}
sv_field_t;

typedef struct sv_field_cache_t
{
  sv_field_t *cache[SV_FIELD_CACHE_SIZE];
}
sv_field_cache_t;

extern int sv_field_cache_add (sv_field_cache_t* m, const char *name, char *body);
extern int sv_field_cache_get (sv_field_cache_t* m, const char *name, const char ***body);
extern void sv_field_cache_release (sv_field_cache_t* m);

#endif

