dnl This file is part of GNU mailutils.
dnl Copyright (C) 2001, 2010-2012 Free Software Foundation, Inc.
dnl
dnl This file is free software; as a special exception the author gives
dnl unlimited permission to copy and/or distribute it, with or without
dnl modifications, as long as this notice is preserved.
dnl
dnl GNU Mailutils is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
dnl implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
dnl
dnl Check for --enable-debug switch. When the switch is specified, add
dnl -ggdb to CFLAGS and remove any optimization options from there.
dnl 

AC_DEFUN([MU_DEBUG_MODE],
  [AC_ARG_ENABLE(debug,                     
    AC_HELP_STRING([--enable-debug], [enable debugging mode]),
    [mu_debug_mode=$enableval],
    [mu_debug_mode=maybe])
    
   save_CC=$CC
   CC="$CC -Wall"
   AC_TRY_COMPILE([],[void main(){}],
                  [CFLAGS="$CFLAGS -Wall"])
		  
   CC="$CC -Wdeclaration-after-statement"
   AC_TRY_COMPILE([],[void main(){}],
                  [CFLAGS="$CFLAGS -Wdeclaration-after-statement"])
   
   if test "$mu_debug_mode" != no; then
     CFLAGS=`echo $CFLAGS | sed 's/-O[[0-9]]//g'`
     AC_MSG_CHECKING([whether cc accepts -ggdb])
     CC="$CC -ggdb"
     AC_TRY_COMPILE([],[void main(){}],
                    [AC_MSG_RESULT(yes)
		     CFLAGS="$CFLAGS -ggdb"],
		    [AC_MSG_RESULT(no)
		     CFLAGS="$CFLAGS -g"])
   fi
   CC=$save_CC])
