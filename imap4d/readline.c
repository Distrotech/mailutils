#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef COUNT
#define COUNT 128 /* safe, middle of the road value */
#endif

/**
 * reads a single line from *stream
 * max line length is max size of size_t
 * 
 * PRE: stream is an open FILE stream
 * POST: returns an allocated string containing the line read, or errno is set
 *
 * note, user must free the returned string
 **/
char *read_a_line(FILE *stream) {

	char *line = NULL;
	size_t buffsize = 0; /* counter of alloc'ed space */

	if( feof(stream) ) { /* sent a bum stream */
		errno = EINVAL;	
		return NULL;
	}

	do {
		if ((line = realloc(line, buffsize + (sizeof(char) * COUNT))) == NULL) {
			errno = ENOMEM;
			return NULL;
		}
		if( fgets(line + (buffsize ? buffsize - 1 : 0),COUNT,stream) == NULL ) {
			if( buffsize == 0 ) {
				errno = EINVAL;
				return NULL; /* eof w/ nothing read */
			}
			break; /* read a line w/o a newline char */
		}
		buffsize += COUNT;
	} while( (strchr(line, '\n')) == NULL );

	return line;
}
