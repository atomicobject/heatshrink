# heatshrink Changes By Release

## v0.4.2 - 2021-xx-yy

### API Changes

None.


### Bug Fixes

Fixed fallthrough in getopt 'h'.


### Other Improvements

Added `CHANGELOG.md`.

Added comment to help explain build error when attempting to build the
command line interface for embedded systems without getopt.h/err.h.

Use constant rather than bare int 4096 for buffer size.

Fixed fallthrough compiler warning.

Move repo into `include/`, `src/`, `test/`, and build in `build/`.

Update theft tests to match interface changes in theft.

greatest.h: Update vendored copy to 1.5.0.

Fix warnings with new version of greatest.


## v0.4.1 - 2015-11-01

### API Changes

None.


### Bug Fixes

Update inaccurate assertion. (Only active when logging.)


### Other Improvements

Fix minor typo.

benchmark: Scrape file size from ls | awk -- stat & du args are less portable.

Update contact info.


## v0.4.0 - 2015-06-07

### API Changes

The limits for the window and lookahead size options have changed. The
minimum lookahead bits option (-l) is now 3 (previously, 2), and the
maximum is the window bits - 1 (previously, equal to the window bits).

Change `get_bits()` to `uint16_t`.


### Bug Fixes

Fix search when window size equals lookahead.

Correct encoder state machine graph.


### Other Improvements

Fix signed/unsigned comparisons.

Use consistently signed vars during index construction.

Fix debug output and asserts.

Vary window and lookahead sizes in theft tests, add a regression test.
Thanks to @unixdj for finding this bug and providing very detailed
info about how to reproduce it!

Simplify break even point calculation a bit further.

Clean up / dead code elimination.

Misc test cleanup.

Add compression benchmarking script.

Initial work on a CONTRIBUTING.md.

README: Add details on example usage and recommended configuration defaults.

Consistently use "decompress", not "uncompress".


## v0.3.2 - 2015-01-03

### API Changes

None.


### Bug Fixes

Fix handling of `O_BINARY` on Windows.


### Other Improvements

Add optional property-based tests, using theft (if available).

Fix compilation on MinGW and other platforms without err.h

Print all status messages on stderr.

Fix missing #include for getopt.h.

Complete overhaul of Makefile: both static & dynamic variant static libs.



## v0.3.1 - 2014-06-25

### API Changes

None.


### Bug Fixes

Fix possible state yielding MORE from finish(), but 0 bytes from poll().


### Other Improvements

Improve non-indexed compression performance.

When indexed, only check matches that can beat the current best match.

Only check for matches that can be improvements.

Update heatshrink_common.h filename in README.md to reflect rename.

Add usage details, homepage to `heatshrink -h` message.

CLI: note that compression is the default.x


## v0.3.0 - 2013-12-28

### API Changes

Update non-indexed matching to use an `int16_t` pos.

Update index type to `int16_t`.

Decrease `HEATSHRINK_MAX_WINDOW_BITS` to 15.


### Bug Fixes

None.


### Other Improvements

Remove unused static library target from Makefile.

Cast `write(2)`'s -1 to (size_t) to avoid sign mismatch warning.

Code style cleanup: _always_ brace `if`s, extract NO_BITS constant.

Eliminate redundant comparison, index ensures first byte matches.

Improve throughput of `find_longest_match` via several improvements
to the indexing strategy.

If calling push_bits w/ 8 bits at beginning of new byte, just copy byte over whole.


## v0.2.1 - 2013-12-18

### API Changes

None.


### Bug Fixes

None.


### Other Improvements

Renamed `heatshrink.h` to `heatshrink_common.h`, to make it more
clearly separate from the `heatshrink.c` CLI interface.

Don't use assert unless `HEATSHRINK_DEBUGGING_LOGS` is set.

Fix some (potentially) unused variable and numeric type warnings.

Add warnings from `-Wextra`.

Add braces around statements hanging off `if`s that may compile out to no-ops.

Fix usage string ("[-v]" not "[v]").

Change from BSD-3 License to ISC License.

Reduce default `static_input_buffer_size`.

Update decoder state machine diagram to reflect MSB/LSB split.



## v0.2.0 - 2013-05-12

### API Changes

Change `uint16_t` to `size_t` for user-controlled buffer sizes.


### Bug Fixes

When index or count are > 8 bits long, ensure suspend/resume is correct.


### Other Improvements

Consistentcy: log to stderr for both enc and dec.

Tighten bounds for lookahead and window size.
Since they're only stored as `uint16_t`s, disallow overly large sizes.


## v0.1.0 - 2013-03-13

Initial public release.
