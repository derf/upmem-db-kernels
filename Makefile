NR_TASKLETS ?= 16
BL ?= 10

aspectc ?= 0
aspectc_timing ?= 0
dfatool_timing ?= 1
numa ?= 0
verbose ?= 0

HOST_CC := ${CC}
UPMEM_CC := ${CC}
DPU_CC := dpu-upmem-dpurte-clang

FLAGS :=
CFLAGS := -Wall -Wextra -pedantic -Iinclude
CPU_CFLAGS := ${CFLAGS} -O3 -march=native -DNUMA=${numa} -DDFATOOL_TIMING=${dfatool_timing}
HOST_CFLAGS := ${CFLAGS} -O3 -march=native $$(dpu-pkg-config --cflags --libs dpu) -DNR_TASKLETS=${NR_TASKLETS} -DBL=${BL} -DNUMA=${numa} -DDFATOOL_TIMING=${dfatool_timing} -DASPECTC=${aspectc}
DPU_CFLAGS := ${CFLAGS} -O2 -DNR_TASKLETS=${NR_TASKLETS} -DBL=${BL}

INCLUDES := $(wildcard include/*.h)
CPU_SOURCES := $(wildcard cpu/*.c)
HOST_SOURCES := $(wildcard host/*.c)
DPU_SOURCES := $(wildcard dpu/*.c)

ifeq (${aspectc_timing}, 1)
	ASPECTC_HOST_FLAGS += -a include/dfatool_cpu.ah
	ASPECTC_UPMEM_FLAGS += -a include/dfatool_host.ah
endif

ASPECTC_HOST_FLAGS ?= -a0
ASPECTC_UPMEM_FLAGS ?= -a0

ifeq (${aspectc}, 1)
	HOST_CC = ag++ -r repo.acp -v 0 ${ASPECTC_HOST_FLAGS} -p . --Xcompiler
	UPMEM_CC = ag++ -r repo.acp -v 0 ${ASPECTC_UPMEM_FLAGS} --c_compiler ${UPMEM_HOME}/bin/clang++ -p . --Xcompiler
else
	CPU_FLAGS += -std=c11
	HOST_FLAGS += -std=c11
endif

ifeq (${numa}, 1)
	FLAGS += -lnuma
endif

QUIET = @

ifeq (${verbose}, 1)
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
	${QUIET}${HOST_CC} ${CPU_CFLAGS} -o $@ ${CPU_SOURCES} ${FLAGS}

bin/host_code: ${HOST_SOURCES} ${INCLUDES}
	${QUIET}mkdir -p bin
	${QUIET}${UPMEM_CC} ${HOST_CFLAGS} -o $@ ${HOST_SOURCES}

bin/dpu_code: ${DPU_SOURCES} ${INCLUDES}
	${QUIET}mkdir -p bin
	${QUIET}${DPU_CC} ${DPU_CFLAGS} -o $@ ${DPU_SOURCES}

clean:
	${QUIET}rm -rf bin

test: all
	${QUIET}bin/host_code

.PHONY: all clean test
