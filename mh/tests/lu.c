/* This file is part of GNU Mailutils test suite.
   Copyright (C) 2012 Free Software Foundation, Inc.

   This file is free software; as a special exception the author gives
   unlimited permission to copy and/or distribute it, with or without
   modifications.

   This file is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/  
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>

/* List all user names.  With a single numeric argument, list users with UID
   greater than that argument. */
int
main (int argc, char **argv)
{
	struct passwd *pw;
	int minuid = 0;
	
	if (argc == 2)
		minuid = atoi (argv[1]);
	while ((pw = getpwent ()))
		if (pw->pw_uid > minuid)
			printf ("%s\n", pw->pw_name);
	return 0;
}
