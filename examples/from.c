/** 
 * Code in the public domain, Sean 'Shaleh' Perry <shaleh@debian.org>, 1999
 *
 * Created as an example of using libmailbox
 *
 **/
#include <mailbox.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	mailbox *mail;
	unsigned int i;
	char *from, *date;
	char *user;
	char mailpath[256];

	user = getenv("USER");
	if (user == NULL)
      {
        fprintf(stderr, "who am I?\n");
        exit(-1);
      }
	snprintf (mailpath, 256, "%s/%s", _PATH_MAILDIR, user);
	mail = mbox_open(mailpath);
	if( mail == NULL ) {
		perror("mbox_open: ");
		exit(-1);
	}
	for(i = 0; i < mail->messages; ++i) {
		from = mbox_header_line(mail, i, "From");
		if (from == NULL)
          {
            perror("mbox_header_line: ");
            exit(-1);
          }
		date = mbox_header_line(mail, i, "Date");
		if (date == NULL)
          {
            perror("mbox_header_line: ");
            exit(-1);
          }
		printf("%s %s\n", from, date);
		free(from);
		free(date);
	}
	mbox_close(mail);
	exit(0);
}
