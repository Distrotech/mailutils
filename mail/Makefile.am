CFLAGS = -Wall -pedantic -g -DTESTING
INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib

bin_PROGRAMS = mail

mail_DEPENDENCIES = ../mailbox/libmailbox.la
mail_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.a -lreadline
mail_SOURCES = alias.c alt.c bang.c cd.c copy.c delete.c discard.c dp.c \
	echo.c edit.c eq.c exit.c file.c folders.c followup.c from.c hash.c \
	headers.c help.c hold.c if.c list.c mail.c mail.h mbox.c next.c pipe.c \
	previous.c print.c printall.c quit.c relist.c reply.c retain.c save.c \
	send.c set.c shell.c size.c source.c top.c touch.c unalias.c \
	undelete.c unset.c util.c visual.c write.c z.c table.c
