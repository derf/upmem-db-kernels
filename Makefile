NR_TASKLETS ?= 16
BL ?= 10

CFLAGS := -Wall -Wextra -pedantic -Iinclude
CPU_FLAGS := ${CFLAGS} -std=c11 -O3 -march=native -fopenmp
HOST_CFLAGS := ${CFLAGS} -std=c11 -O3 $$(dpu-pkg-config --cflags --libs dpu) -DNR_TASKLETS=${NR_TASKLETS} -DBL=${BL}
DPU_CFLAGS := ${CFLAGS} -O2 -DNR_TASKLETS=${NR_TASKLETS} -DBL=${BL}

INCLUDES := $(wildcard include/*.h)
CPU_SOURCES := $(wildcard cpu/*.c)
HOST_SOURCES := $(wildcard host/*.c)
DPU_SOURCES := $(wildcard dpu/*.c)

QUIET = @

ifdef verbose
	QUIET =
endif

all: bin/cpu_code bin/host_code bin/dpu_code

bin:
	${QUIET}mkdir -p bin

bin/cpu_code: bin ${CPU_SOURCES} ${INCLUDES}
	${QUIET}${CC} ${CPU_FLAGS} -o $@ ${CPU_SOURCES}

bin/host_code: bin ${HOST_SOURCES} ${INCLUDES}
	${QUIET}${CC} ${HOST_CFLAGS} -o $@ ${HOST_SOURCES}

bin/dpu_code: bin ${DPU_SOURCES} ${INCLUDES}
	${QUIET}dpu-upmem-dpurte-clang ${DPU_CFLAGS} -o $@ ${DPU_SOURCES}

clean:
	${QUIET}rm -rf bin

test: all
	${QUIET}bin/host_code

.PHONY: all clean test
