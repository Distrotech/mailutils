# This file is part of GNU Mailutils
# Copyright (C) 2000,2001, Free Software Foundation

info_TEXINFOS = mailutils.texi
mailutils_TEXINFOS = \
 address.texi \
 attribute.texi \
 auth.texi \
 body.texi \
 c-api.texi \
 encoding.texi \
 envelope.texi \
 ex-address.texi \
 ex-url.texi \
 framework.texi \
 folder.texi \
 headers.texi \
 imap4.texi \
 iterator.texi \
 locker.texi \
 mailbox.texi \
 mailer.texi \
 maildir.texi \
 mbox.texi \
 message.texi \
 mh.texi \
 nntp.texi \
 parse822.texi \
 pop3.texi \
 programs.texi \
 sendmail.texi \
 sfrom.c.texi \
 smtp.texi \
 stream.texi \
 url.texi

## Sam, the examples dir is not part of the distribution so this will not work
## but we should probably have a "addrmsg" program and reenable this.
## Alain.
#ex-address.texi: ${top_srcdir}/doc/examples/addr.c
#	sed -es/{/@{/g -e s/}/@}/g < $< > $@
#sfrom.texi: ${top_srcdir}/doc/examples/sfrom.c
#	sed -es/{/@{/g -e s/}/@}/g < $< > $@
#ex-url.texi: ${top_srcdir}/doc/examples/url-parse.c
#	sed -es/{/@{/g -e s/}/@}/g < $< > $@

#mailutils.info: mailutils.texi version.texi ex-address.texi
#mailutils.dvi: mailutils.texi version.texi ex-address.texi
DISTCLEANFILES=*.pgs
