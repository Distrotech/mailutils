AUTOMAKE_OPTIONS = ../lib/ansi2knr no-dependencies

CFLAGS = -Wall -g

INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib @ARGPINCS@

bin_PROGRAMS = mail

mail_LDADD = @ARGPLIBS@ ../mailbox/libmailbox.la ../lib/libmailutils.a

mail_SOURCES = alias.c alt.c cd.c copy.c delete.c discard.c dp.c echo.c \
  edit.c eq.c exit.c file.c folders.c followup.c from.c headers.c help.c \
  hold.c if.c list.c mail.c mail.h mbox.c next.c pipe.c previous.c print.c \
  quit.c reply.c retain.c save.c send.c set.c shell.c size.c source.c table.c \
  top.c touch.c unalias.c undelete.c unset.c util.c visual.c write.c z.c
