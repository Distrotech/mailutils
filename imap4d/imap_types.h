#ifndef _MY_IMAP_TYPES
#define _MY_IMAP_TYPES

/* possible server states */
enum _STATES {NON_AUTH, AUTH, SELECTED, LOGOUT};

/* imap command status codes */
enum _STATUS {BAD, NO, OK};

typedef enum _STATES STATES;
typedef enum _STATUS STATUS;

typedef struct _Command {
	char tag[16]; 
	char cmd[16];
	char *args;
} Command;

#endif
