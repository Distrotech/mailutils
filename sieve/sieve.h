
/** mailutils' sieve implementation */
/*

Configuration:

  debug level

  print callback and callback pointer

  ticket_t

  "no actions" mode

Basic methods:

  create and configure context

  parse script

  sieve message

Extended methods:

  filter mailbox

  filter mailbox URL

  set ticket_t based on wicket file

*/

#include <errno.h>

#include <mailutils/message.h>
#include <mailutils/debug.h>

/* Sieve specific intepretations of error numbers. */
#define SV_EPARSE ENOEXEC

extern const char* sv_strerror(int e);

typedef struct sv_interp_ctx_t* sv_interp_t;
typedef struct sv_script_ctx_t* sv_script_t;

typedef void (*sv_parse_error_t)(const char* script, int lineno, const char* errmsg);
typedef void (*sv_execute_error_t)(const char* script, message_t msg, int rc, const char* errmsg);
typedef void (*sv_action_log_t)(const char* script, message_t msg, const char* action, const char* fmt, va_list ap);

extern int sv_interp_alloc(sv_interp_t *i,
               sv_parse_error_t pe,
	       sv_execute_error_t ee,
	       sv_action_log_t al);

extern void sv_interp_free(sv_interp_t *i);

extern int sv_script_parse(sv_script_t *s, sv_interp_t i, const char *script);
extern void sv_script_free(sv_script_t *s);

#define SV_FLAG_NO_ACTIONS 0x01

/* Sieve specific debug levels - can be set in the mu_debug_t. */
#define SV_DEBUG_TRACE      0x100
#define SV_DEBUG_HDR_FILL   0x200
#define SV_DEBUG_MSG_QUERY  0x400

extern int sv_script_execute(sv_script_t s, message_t m, ticket_t t, mu_debug_t d, int svflags);

