NR_TASKLETS ?= 16
BL ?= 10

dfatool_timing ?= 1
numa ?= 0

FLAGS :=
CFLAGS := -Wall -Wextra -pedantic -Iinclude
CPU_CFLAGS := ${CFLAGS} -std=c11 -O3 -march=native -DNUMA=${numa}
HOST_CFLAGS := ${CFLAGS} -std=c11 -O3 -march=native $$(dpu-pkg-config --cflags --libs dpu) -DNR_TASKLETS=${NR_TASKLETS} -DBL=${BL} -DNUMA=${numa} -DDFATOOL_TIMING=${dfatool_timing}
DPU_CFLAGS := ${CFLAGS} -O2 -DNR_TASKLETS=${NR_TASKLETS} -DBL=${BL}

INCLUDES := $(wildcard include/*.h)
CPU_SOURCES := $(wildcard cpu/*.c)
HOST_SOURCES := $(wildcard host/*.c)
DPU_SOURCES := $(wildcard dpu/*.c)

ifeq (${numa}, 1)
	FLAGS += -lnuma
endif

QUIET = @

ifdef verbose
	QUIET =
endif

ifndef chios
	CPU_CFLAGS += -fopenmp -DHAVE_OMP=1
	HOST_CFLAGS += -fopenmp -DHAVE_OMP=1
endif

ifdef tinos
	CPU_CFLAGS += -I/usr/lib/llvm-12/lib/clang/12.0.1/include -L/usr/lib/llvm-12/lib/ -Wno-gnu-zero-variadic-macro-arguments
	HOST_CFLAGS += -I/usr/lib/llvm-12/lib/clang/12.0.1/include -L/usr/lib/llvm-12/lib/ -Wno-gnu-zero-variadic-macro-arguments
endif

all: bin/cpu_code bin/host_code bin/dpu_code

bin/cpu_code: ${CPU_SOURCES} ${INCLUDES}
	${QUIET}mkdir -p bin
	${QUIET}${CC} ${CPU_CFLAGS} -o $@ ${CPU_SOURCES} ${FLAGS}

bin/host_code: ${HOST_SOURCES} ${INCLUDES}
	${QUIET}mkdir -p bin
	${QUIET}${CC} ${HOST_CFLAGS} -o $@ ${HOST_SOURCES}

bin/dpu_code: ${DPU_SOURCES} ${INCLUDES}
	${QUIET}mkdir -p bin
	${QUIET}dpu-upmem-dpurte-clang ${DPU_CFLAGS} -o $@ ${DPU_SOURCES}

clean:
	${QUIET}rm -rf bin

test: all
	${QUIET}bin/host_code

.PHONY: all clean test
