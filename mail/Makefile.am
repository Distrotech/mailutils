AUTOMAKE_OPTIONS = ../lib/ansi2knr 
AM_YFLAGS=-vt
INCLUDES = -I${top_srcdir}/include -I${top_srcdir}/lib 
AM_CFLAGS = -DSITE_MAIL_RC=\"@SITE_MAIL_RC@\"

bin_PROGRAMS = mail

mail_LDADD = ../mailbox/libmailbox.la ../lib/libmailutils.la \
             @READLINE_LIBS@ 

EXTRA_DIST = msgset.c

mail_SOURCES = alias.c alt.c cd.c copy.c delete.c dp.c echo.c \
  edit.c eq.c exit.c file.c folders.c followup.c from.c headers.c help.c \
  hold.c if.c inc.c list.c mail.c mail.h mailline.c mbox.c msgset.y next.c \
  pipe.c previous.c print.c quit.c reply.c retain.c save.c send.c set.c \
  shell.c size.c source.c summary.c table.c tag.c top.c touch.c unalias.c \
  undelete.c unset.c util.c var.c version.c visual.c write.c z.c decode.c

SUBDIRS = testsuite
