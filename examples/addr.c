#include <stdio.h>
#include <mailutils/address.h>

static int parse(const char* str)
{
    size_t no = 0;
    size_t pcount;

    char buf[BUFSIZ];

    address_t  address = NULL;

    address_create(&address, str);

    address_get_count(address, &pcount);

    printf("%s=> pcount %d\n", str, pcount);

    for(no = 1; no <= pcount; no++) {
      size_t got = 0;
      printf("%d ", no);

      address_get_email(address, no, buf, sizeof(buf), 0);

      printf("email <%s>\n", buf);

      address_get_personal(address, no, buf, sizeof(buf), &got);

      if(got) printf("   personal <%s>\n", buf);

      address_get_comments(address, no, buf, sizeof(buf), &got);

      if(got) printf("   comments <%s>\n", buf);

      address_get_local_part(address, no, buf, sizeof(buf), &got);

      if(got) printf("   local-part <%s>", buf);

      address_get_domain(address, no, buf, sizeof(buf), &got);

      if(got) printf(" domain <%s>\n", buf);

      address_get_route(address, no, buf, sizeof(buf), &got);

      if(got) printf("   route <%s>\n", buf);
    }
    address_destroy(&address);

    printf("\n");

    return 0;
}

static int parseinput(void)
{
	char buf[BUFSIZ];

	while(fgets(buf, sizeof(buf), stdin) != 0) {
		buf[strlen(buf) - 1] = 0;
		parse(buf);
	}

	return 0;
}

int main(int argc, const char *argv[])
{
	argc = 1;

	if(!argv[argc]) {
		return parseinput();
	}
	for(; argv[argc]; argc++) {
		if(strcmp(argv[argc], "-") == 0) {
			parseinput();
		} else {
			parse(argv[argc]);
		}
	}

	return 0;
}
