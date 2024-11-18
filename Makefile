NR_TASKLETS ?= 16
BL ?= 10

CFLAGS := -Wall -Wextra -Iinclude
HOST_CFLAGS := ${CFLAGS} -std=c11 -O3 $$(dpu-pkg-config --cflags --libs dpu) -ggdb -DNR_TASKLETS=${NR_TASKLETS} -DBL=${BL}
DPU_CFLAGS := ${CFLAGS} -O2 -DNR_TASKLETS=${NR_TASKLETS} -DBL=${BL}

HOST_SOURCES := $(wildcard host/*.c)
DPU_SOURCES := $(wildcard dpu/*.c)

QUIET = @

ifdef verbose
	QUIET =
endif

all: bin/host_code bin/dpu_code

bin:
	${QUIET}mkdir -p bin

bin/host_code: bin ${HOST_SOURCES}
	${QUIET}${CC} ${HOST_CFLAGS} -o $@ ${HOST_SOURCES}

bin/dpu_code: bin ${DPU_SOURCES}
	${QUIET}dpu-upmem-dpurte-clang ${DPU_CFLAGS} -o $@ ${DPU_SOURCES}

clean:
	${QUIET}rm -rf bin

test: all
	${QUIET}bin/host_code

.PHONY: all clean test
