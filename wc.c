#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/*
 * count bits set
 * use popcount CPU instruction if available, or
 * https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
 * otherwise
 */
static uint64_t popcount(uint64_t v)
{
#ifndef __POPCNT__
	v = v - ((v >> 1) & 0x5555555555555555);
	v = (v & 0x3333333333333333) + ((v >> 2) & 0x3333333333333333);
	return (((v + (v >> 4)) & 0x0F0F0F0F0F0F0F0F) * 0x0101010101010101) >> 56;
#else	/* __POPCNT__ */
	return __builtin_popcount(v);
#endif	/* __POPCNT__ */
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

	/* mmap() input file with readahead if available (Linux) */
#ifndef MAP_POPULATE
#define MAP_POPULATE	0
#endif
	const char *base = mmap(0, stat.st_size, PROT_READ, MAP_PRIVATE|MAP_POPULATE, fd, 0);
	if (MAP_FAILED == base) exit_err("mmap() failed");

	/*
	 * count newlines and words, 8-bytes at a time
	 * this can overflow (go beyond file size) by at most 7 bytes,
	 * however mmap() guarantees us:
	 *    1) it is page-aligned (greater than 8-bytes)
	 *    2) extra bytes are initialized to 0
	 */
	size_t lc = 0;
	size_t wc = 0;
	uint8_t prev = 0x80;
	for (off_t i=0; i<stat.st_size; i+=8) {
		const uint64_t *vx8 = (void *)&base[i];
		/*
		 * match whitespaces
		 * see https://graphics.stanford.edu/~seander/bithacks.html#ValueInWord
		 */
#define NORMx8(v)		(((v) - 0x0101010101010101UL) & ~(v) & 0x8080808080808080UL)
#define CHARx8(c)		(~0UL/255 * (c))
#define MATCHx8(v, c)	NORMx8((v) ^ CHARx8(c))
		const uint64_t nlx8 = MATCHx8(*vx8, '\n');
		const uint64_t htx8 = MATCHx8(*vx8, '\t');
		const uint64_t crx8 = MATCHx8(*vx8, '\r');
		const uint64_t vtx8 = MATCHx8(*vx8, '\v');
		const uint64_t ffx8 = MATCHx8(*vx8, '\f');
		const uint64_t spx8 = MATCHx8(*vx8, ' ' );

		/* count newlines */
		lc += popcount(nlx8);

		/* get all whitespaces combined for word count */
		const uint64_t wsx8 = nlx8 | htx8 | crx8 | vtx8 | ffx8 | spx8;
		/*
		 * count words: a new word is a transition from 0 (non-whitespace)
		 * to 0x80 (whitespace)
		 */
		wc += popcount((wsx8 ^ ((wsx8 << 8) | prev)) & wsx8);
		prev = wsx8 >> 56;
	}

	/*
	 * if the file ends up with a non-whitespace character, the last word
	 * must be accounted for
	 */
	switch (base[stat.st_size-1]) {
		case '\n':
		case '\t':
		case '\r':
		case '\v':
		case '\f':
		case ' ':
			break;
		default:
			wc++;
	}

	printf("%zu %zu %zu %s\n", lc, wc, stat.st_size, argv[1]);
	return EXIT_SUCCESS;
}
