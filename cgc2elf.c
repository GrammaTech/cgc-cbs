
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <err.h>
#include <unistd.h>

#define	C_IDENT "\177ELF\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00"

int
main(int argc, char *argv[])
{
	int i, f;
	int ret = 0;

	if (argc < 2) {
		fprintf(stderr, "files...\n");
		return (1);
	}

	for (i = 1; i < argc; i++) {
		f = open(argv[i], O_RDWR, 0);
		if (f == -1) {
			ret = 1;
			warn("open %s", argv[i]);
			goto again;
		}

		write(f, C_IDENT, 16);

again:
		if (f != -1) {
			close(f);
			f = -1;
		}
	}

	return (ret);
}
