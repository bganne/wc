#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

/* unrolling factor */
#define N	4
#define xN	for (int n=0; n<N; n++)

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
static inline uint64_t popcount(uint64_t v)
{
	v = v - ((v >> 1) & 0x5555555555555555);
	v = (v & 0x3333333333333333) + ((v >> 2) & 0x3333333333333333);
	return (((v + (v >> 4)) & 0x0F0F0F0F0F0F0F0F) * 0x0101010101010101) >> 56;
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
	/* round-up the mapping to allow overflow (see below) */
	const uint64_t *restrict base = mmap(0, stat.st_size+N*8-8, PROT_READ, MAP_PRIVATE|MAP_POPULATE, fd, 0);
	if (MAP_FAILED == base) exit_err("mmap() failed");

	/*
	 * count newlines and words, 8-bytes at a time, with unrolling
	 * this can overflow (go beyond file size) by at most N-1 8-bytes,
	 * however mmap() guarantees us:
	 *    1) it is page-aligned (pages are bigger than 8-bytes)
	 *    2) extra bytes are initialized to 0
	 * as we added N-1 bytes to the mapping (see above) this is fine
	 */
	size_t lc_[N] = {};
	size_t wc_[N] = {};
	uint8_t prev_ = 0;
	for (off_t i=0; i<(stat.st_size+7)/8; i+=N) {
		/*
		 * match whitespaces
		 * see https://graphics.stanford.edu/~seander/bithacks.html#ValueInWord
		 */
		uint64_t nlx8[N], htx8[N], crx8[N], vtx8[N], ffx8[N], spx8[N];
#define NORMx8(v)		(((v) - 0x0101010101010101UL) & ~(v) & 0x8080808080808080UL)
#define CHARx8(c)		(~0UL/255 * (c))
#define MATCHx8(v, c)	NORMx8((v) ^ CHARx8(c))
		xN nlx8[n] = MATCHx8(base[i+n], '\n');
		xN htx8[n] = MATCHx8(base[i+n], '\t');
		xN crx8[n] = MATCHx8(base[i+n], '\r');
		xN vtx8[n] = MATCHx8(base[i+n], '\v');
		xN ffx8[n] = MATCHx8(base[i+n], '\f');
		xN spx8[n] = MATCHx8(base[i+n], ' ' );

		/* count newlines */
		xN lc_[n] += popcount(nlx8[n]);

		/*
		 * count words: a new word is a transition from 0 (non-whitespace)
		 * to 0x80 (whitespace)
		 */
		uint64_t wsx8[N];
		xN wsx8[n] = nlx8[n] | htx8[n] | crx8[n] | vtx8[n] | ffx8[n] | spx8[n];

		uint8_t prev[N+1];
		prev[0] = prev_;
		xN prev[n+1] = wsx8[n] >> 56;
		prev_ = prev[N];

		uint64_t tmp[N];
		xN tmp[n] = wsx8[n] << 8;
		xN tmp[n] |= prev[n];
		xN tmp[n] &= wsx8[n];
		xN tmp[n] ^= wsx8[n];

		xN wc_[n] += popcount(tmp[n]);
	}

	size_t lc = 0, wc = 0;
	xN lc += lc_[n];
	xN wc += wc_[n];

	/*
	 * if the file ends up with a non-whitespace character, the last word
	 * must be accounted for
	 */
	switch (((char *)base)[stat.st_size-1]) {
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
