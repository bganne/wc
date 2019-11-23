#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

static void exit_err(const char *str)
{
	perror(str);
	exit(EXIT_FAILURE);
}

int main(int argc, const char **argv)
{
	if (2 != argc) {
		fprintf(stderr, "Usage: %s <path>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) exit_err("open() failed");

	struct stat stat;
	int err = fstat(fd, &stat);
	if (err) exit_err("fstat() failed");

	const char *base = mmap(0, stat.st_size, PROT_READ, MAP_PRIVATE|MAP_POPULATE, fd, 0);
	if (MAP_FAILED == base) exit_err("mmap() failed");

	int ws = 1;
	size_t wc = 0;
	size_t lc = 0;
	for (size_t i=0; i<stat.st_size; i++) {
		switch (base[i]) {
			case ' ' :
			case '\t':
			case '\r':
			case '\v':
			case '\f':
				ws = 1;
				break;
			case '\n':
				ws = 1;
				lc++;
				break;
			default:
				if (ws) {
					wc++;
					ws = 0;
				}
				break;
		}
	}

	printf("%zu %zu %zu %s\n", lc, wc, stat.st_size, argv[1]);
	return EXIT_SUCCESS;
}
