PROJECT = heatshrink
#OPTIMIZE = -O0
#OPTIMIZE = -Os
OPTIMIZE = -O3
WARN = -Wall -Wextra -pedantic #-Werror
CFLAGS += -std=c99 -g ${WARN} ${OPTIMIZE}
CFLAGS += -Wmissing-prototypes
CFLAGS += -Wstrict-prototypes
CFLAGS += -Wmissing-declarations

all:
	@echo "For tests, make test_heatshrink_dynamic (default) or change the"
	@echo "config.h to disable static memory and build test_heatshrink_static."
	@echo "For the standalone command-line tool, make heatshrink."

${PROJECT}: heatshrink.c

heatshrink: heatshrink_encoder.o heatshrink_decoder.o
test_heatshrink_dynamic: heatshrink_encoder.o heatshrink_decoder.o
test_heatshrink_static: heatshrink_encoder.o heatshrink_decoder.o

*.o: Makefile heatshrink_config.h

heatshrink_decoder.o: heatshrink_decoder.h
heatshrink_encoder.o: heatshrink_encoder.h

tags: TAGS

TAGS:
	etags *.[ch]

diagrams: dec_sm.png enc_sm.png

dec_sm.png: dec_sm.dot
	dot -o $@ -Tpng $<

enc_sm.png: enc_sm.dot
	dot -o $@ -Tpng $<

clean:
	rm -f ${PROJECT} test_heatshrink_{dynamic,static} *.o *.core {dec,enc}_sm.png TAGS
