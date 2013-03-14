#heatshrink

A data compression/decompression library for
embedded/real-time systems.


## Key Features:

- **Low memory usage (as low as 50 bytes)**      
    It is useful for some cases with less than 50 bytes and useful
    for more general cases with <300 bytes
- **Incremental bounded CPU use**  
    You can chew on the data in as small of pieces as you would like.
    This is a useful property in hard-realtime environments
- **Configurable to use either static or dynamic memory allocation**      
    The library is quite flexible in how it lets you allocate its
    memory.
- **BSD license**      
    You can use it freely even for commercial purposes

## Getting Started:

There is a standalone command-line program, `heatshrink`, but the
encoder and decoder can also be used as libraries, independent of each
other. To do so, copy heatshrink.h, `heatshrink_config.h`, and either
`heatshrink_encoder.c` or `heatshrink_decoder.c` (and their respective
header) into your project.

Dynamic allocation is used by default, but in an embedded context, you
probably want to statically allocate the encoder/decoder. Set
`HEATSHRINK_DYNAMIC_ALLOC` to 0 in `heatshrink_config.h`.

## More Information and Benchmarks:

heatshrink is based on [LZSS], since it's particularly suitable for
compression in small amounts of memory. It can use an optional index to
make compression faster, but otherwise can run in under 100 bytes of
memory.

For more information, see the [blog post] for an overview, and the
`heatshrink_encoder.h` / `heatshrink_decoder.h` header files for API
documentation.

[blog post]: http://spin.atomicobject.com/2013/03/14/heatshrink-embedded-data-compression/
[LZSS]: http://en.wikipedia.org/wiki/Lempel-Ziv-Storer-Szymanski
