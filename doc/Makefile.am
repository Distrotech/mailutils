info_TEXINFOS = mailutils.texi

EXTRA_DIST = \
 rfc1734.txt \
 rfc1939.txt \
 rfc1957.txt \
 rfc2060.txt \
 rfc2060-errata \
 rfc2449.txt \
 address.texi \
 attribute.texi \
 auth.texi \
 body.texi \
 encoding.texi \
 ex-address.texi \
 headers.texi \
 locker.texi \
 mailbox.texi \
 mailer.texi \
 message.texi \
 sfrom.c.texi \
 stream.texi \
 url.texi \
 version.texi

## Sam, the examples dir is not part of the distribution so this will not work
## but we should probably have a "addrmsg" program and reenable this.
## Alain.
#ex-address.texi: ${top_srcdir}/doc/examples/addr.c
#	sed -es/{/@{/g -e s/}/@}/g < $< > $@
#sfrom.texi: ${top_srcdir}/doc/examples/sfrom.c
#	sed -es/{/@{/g -e s/}/@}/g < $< > $@

#mailutils.info: mailutils.texi version.texi ex-address.texi
#mailutils.dvi: mailutils.texi version.texi ex-address.texi
