#include <stdio.h>
#include <errno.h>
#include <mailutils/address.h>

#define EPARSE ENOENT

static const char* err_name(int e)
{
    struct {
	int e;
	const char* s;
    } map[] = {
#define E(e) { e, #e },
	E(ENOENT)
	E(EINVAL)
	E(ENOMEM)
#undef E
	{ 0, NULL }
    };
    static char s[sizeof(int) * 8 + 3];
    int i;

    for(i = 0; map[i].s; i++) {
	if(map[i].e == e)
	    return map[i].s;
    }
    sprintf(s, "[%d]", e);

    return s;
}

static int parse(const char* str)
{
    size_t no = 0;
    size_t pcount = 0;
    int status;

    char buf[BUFSIZ];

    address_t  address = NULL;

    status = address_create(&address, str);

    address_get_count(address, &pcount);

    if(status) {
	printf("%s=> error %s\n\n", str, err_name(status));
	return 0;
    } else {
	printf("%s=> pcount %d\n", str, pcount);
    }

    for(no = 1; no <= pcount; no++) {
	size_t got = 0;
	int isgroup;

	address_is_group(address, no, &isgroup);

	printf("%d ", no);

	if(isgroup) {
	  address_get_personal(address, no, buf, sizeof(buf), &got);

	  printf("group <%s>\n", buf);
	} else {
	  address_get_email(address, no, buf, sizeof(buf), 0);

	  printf("email <%s>\n", buf);
	}

	address_get_personal(address, no, buf, sizeof(buf), &got);

	if(got && !isgroup) printf("   personal <%s>\n", buf);

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
