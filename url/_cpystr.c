#include <url0.h>
#include <string.h>

int
_cpystr (const char * src, char * dst, unsigned int size)
{
    int len = src ? strlen (src) : 0 ;

    if (dst == NULL || size == 0) {
	return len;
    }

    if (len >= size) {
	len = size - 1;
    }
    memcpy (dst, src, len);
    dst[len] = '\0';
    return len;
}
