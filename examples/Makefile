#OPTIMIZE = -ggdb -O0
OPTIMIZE = -O3

all: simple_encoder simple_decoder
	
simple_encoder: simple_encoder.c ../heatshrink_encoder.c
	${CC} -Wall ${OPTIMIZE} -o $@ $^

simple_decoder: simple_decoder.c ../heatshrink_decoder.c
	${CC} -Wall ${OPTIMIZE} -o $@ $^

clean:
	rm -rf simple_encoder simple_decoder
