PROJECT=	heatshrink
OPTIMIZE=	-O3
WARN = 		-Wall -Wextra -pedantic #-Werror
WARN += 	-Wmissing-prototypes
WARN += 	-Wstrict-prototypes
WARN += 	-Wmissing-declarations

BUILD =		build
SRC =		src
INCLUDE =	include
TEST =		test
VENDOR =	vendor

# If libtheft is available, build additional property-based tests.
# Uncomment these to use it in test_heatshrink_dynamic.
THEFT_PATH=	/usr/local/
THEFT_INC=	-I${THEFT_PATH}/include/
# CFLAGS += 	-DHEATSHRINK_HAS_THEFT
# LDFLAGS +=	-L${THEFT_PATH}/lib -ltheft

INC=		-I${INCLUDE} -I${SRC}
CFLAGS += -std=c99 -g ${WARN} ${THEFT_INC} ${INC} ${OPTIMIZE}

all: ${BUILD}/heatshrink test_runners libraries

libraries: ${BUILD}/libheatshrink_static.a ${BUILD}/libheatshrink_dynamic.a

test_runners: ${BUILD}/test_heatshrink_static ${BUILD}/test_heatshrink_dynamic

test: test_runners
	${BUILD}/test_heatshrink_static
	${BUILD}/test_heatshrink_dynamic

ci: test

clean:
	rm -rf ${BUILD}
	rm -rf ${BENCHMARK_OUT}
	rm -f TAGS

TAGS:
	etags *.[ch]

diagrams: ${BUILD}/dec_sm.png ${BUILD}/enc_sm.png

${BUILD}/%.png: ${SRC}/%.dot
	dot -o $@ -Tpng $<


# Benchmarking
CORPUS_ARCHIVE=	cantrbry.tar.gz
CORPUS_URL=	http://corpus.canterbury.ac.nz/resources/${CORPUS_ARCHIVE}
BENCHMARK_OUT=	${BUILD}/benchmark_out

## Uncomment one of these.
DL=	curl -o ${BUILD}/${CORPUS_ARCHIVE}
#DL=	wget -O ${BUILD}/${CORPUS_ARCHIVE}

bench: ${BUILD}/heatshrink corpus
	mkdir -p ${BENCHMARK_OUT}
	cd ${BENCHMARK_OUT} && tar vzxf ../${CORPUS_ARCHIVE}
	time test/benchmark

corpus: ${BUILD}/${CORPUS_ARCHIVE}

${CORPUS_ARCHIVE}:
	${DL} ${CORPUS_URL}

# Installation
PREFIX ?=	/usr/local
INSTALL ?=	install
RM ?=		rm

install: libraries heatshrink
	${INSTALL} -c heatshrink ${PREFIX}/bin/
	${INSTALL} -c libheatshrink_static.a ${PREFIX}/lib/
	${INSTALL} -c libheatshrink_dynamic.a ${PREFIX}/lib/
	${INSTALL} -c heatshrink_common.h ${PREFIX}/include/
	${INSTALL} -c heatshrink_config.h ${PREFIX}/include/
	${INSTALL} -c heatshrink_encoder.h ${PREFIX}/include/
	${INSTALL} -c heatshrink_decoder.h ${PREFIX}/include/

uninstall:
	${RM} -f ${PREFIX}/lib/libheatshrink_static.a
	${RM} -f ${PREFIX}/lib/libheatshrink_dynamic.a
	${RM} -f ${PREFIX}/include/heatshrink_common.h
	${RM} -f ${PREFIX}/include/heatshrink_config.h
	${RM} -f ${PREFIX}/include/heatshrink_encoder.h
	${RM} -f ${PREFIX}/include/heatshrink_decoder.h

# Internal targets and rules

OBJS = heatshrink_encoder.o heatshrink_decoder.o

DYNAMIC_OBJS= 	${BUILD}/dynamic/heatshrink_decoder.o \
		${BUILD}/dynamic/heatshrink_encoder.o \

STATIC_OBJS= 	${BUILD}/static/heatshrink_decoder.o \
		${BUILD}/static/heatshrink_encoder.o \

DYNAMIC_LDFLAGS= ${LDFLAGS} -L${BUILD} -lheatshrink_dynamic
STATIC_LDFLAGS= ${LDFLAGS} -L${BUILD} -lheatshrink_static

# Libraries should be built separately for versions
# with and without dynamic allocation.
CFLAGS_STATIC = ${CFLAGS} -I${VENDOR} -DHEATSHRINK_DYNAMIC_ALLOC=0
CFLAGS_DYNAMIC = ${CFLAGS} -I${VENDOR} -DHEATSHRINK_DYNAMIC_ALLOC=1

${BUILD}/heatshrink: ${BUILD}/heatshrink.o ${BUILD}/libheatshrink_dynamic.a
	${CC} -o $@ $^ ${CFLAGS_DYNAMIC} -L${BUILD} -lheatshrink_dynamic

TEST_OBJS_DYNAMIC=	${BUILD}/test_heatshrink_dynamic.o \
			${BUILD}/test_heatshrink_dynamic_theft.o \

TEST_OBJS_STATIC=	${BUILD}/test_heatshrink_static.o \


${BUILD}/test_heatshrink_dynamic: ${TEST_OBJS_DYNAMIC} ${BUILD}/libheatshrink_dynamic.a
	${CC} -o $@ ${TEST_OBJS_DYNAMIC} ${CFLAGS_DYNAMIC} ${DYNAMIC_LDFLAGS}

${BUILD}/test_heatshrink_static: ${TEST_OBJS_STATIC} ${BUILD}/libheatshrink_static.a
	${CC} -o $@ $< ${CFLAGS_STATIC} ${STATIC_LDFLAGS}

${BUILD}/libheatshrink_static.a: ${STATIC_OBJS}
	ar -rcs $@ $^

${BUILD}/libheatshrink_dynamic.a: ${DYNAMIC_OBJS}
	ar -rcs $@ $^

${BUILD}/dynamic/%.o: ${SRC}/%.c | ${BUILD}/dynamic/
	${CC} -c -o $@ $< ${CFLAGS_DYNAMIC}

${BUILD}/static/%.o: ${SRC}/%.c | ${BUILD}/static/
	${CC} -c -o $@ $< ${CFLAGS_STATIC}

${BUILD}/%.o: ${SRC}/%.c | ${BUILD}
	${CC} -c -o $@ $< ${CFLAGS_DYNAMIC}

${BUILD}/test_heatshrink_static.o: ${TEST}/test_heatshrink_static.c | ${BUILD}
	${CC} -c -o $@ $< ${CFLAGS_STATIC}

${BUILD}/test_heatshrink_dynamic.o: ${TEST}/test_heatshrink_dynamic.c | ${BUILD}
	${CC} -c -o $@ $< ${CFLAGS_DYNAMIC}

${BUILD}/test_heatshrink_dynamic_theft.o: ${TEST}/test_heatshrink_dynamic_theft.c | ${BUILD}
	${CC} -c -o $@ $< ${CFLAGS_DYNAMIC}

${BUILD}:
	mkdir ${BUILD}

${BUILD}/static/: | ${BUILD}
	mkdir ${BUILD}/static

${BUILD}/dynamic/: | ${BUILD}
	mkdir ${BUILD}/dynamic

${BUILD}/*.o: Makefile ${INCLUDE}/*.h ${SRC}/*.h


