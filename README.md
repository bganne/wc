# Beat them all with 80 lines of C

I was inspired by the constant stream of post on [HN](https://hn.algolia.com/?q=beating+c+with) about beating C with X, where X is the author favorite pet language.
I wanted it to be idiomatic but not too complex (no hand-optimized vectorization for instance). This is a typical high-performance portable C code.
It is not multi-threaded, because who needs it when 1 core is enough ? (Adding multi-threading with OpenMP is trivial though, if needs arise).

## TL;DR

	make
	./wc <file>

## Performance

On my old system (Thinkpad X61 / Core2 Duo T7300 @2.00GHz / DDR2), it exhibits around 2.5-cycles/byte, eg. processing 750MB in roughly 1.5s when the file is in the page cache.
I am confident it can beat the pants off of multi-threaded implementations in those other toy languages :)
