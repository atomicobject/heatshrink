#ifndef HEATSHRINK_CONFIG_H
#define HEATSHRINK_CONFIG_H

/* Should functionality assuming dynamic allocation be used? */
#define HEATSHRINK_DYNAMIC_ALLOC 1

#if HEATSHRINK_DYNAMIC_ALLOC
    /* Optional replacement of malloc/free */
    #define HEATSHRINK_MALLOC(SZ) malloc(SZ)
    #define HEATSHRINK_FREE(P, SZ) free(P)
#else
    /* Required parameters for static configuration */
    #define HEATSHRINK_STATIC_INPUT_BUFFER_SIZE 32
    #define HEATSHRINK_STATIC_WINDOW_BITS 8
    #define HEATSHRINK_STATIC_LOOKAHEAD_BITS 4
#endif

#ifndef HEATSHRINK_NEEDS_EXPLICIT_ERRNO
/* Either define this outside of the configuration as build parameter or
 * it will be chosen automatically.
 */
#if _WIN32 || __MICROBLAZE__
#define HEATSHRINK_NEEDS_EXPLICIT_ERRNO 1
#else
#define HEATSHRINK_NEEDS_EXPLICIT_ERRNO 0
#endif
#endif

/* Turn on logging for debugging. */
#define HEATSHRINK_DEBUGGING_LOGS 0

/* Use indexing for faster compression. (This requires additional space.) */
#define HEATSHRINK_USE_INDEX 1

#endif
