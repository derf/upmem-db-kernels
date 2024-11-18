#pragma once

// Transfer size between MRAM and WRAM
#ifdef BL
#define BLOCK_SIZE_LOG2 BL
#define BLOCK_SIZE (1 << BLOCK_SIZE_LOG2)
#else
#define BLOCK_SIZE_LOG2 10
#define BLOCK_SIZE (1 << BLOCK_SIZE_LOG2)
#define BL BLOCK_SIZE_LOG2
#endif

#define T uint64_t

typedef struct {
	uint32_t size;
	enum kernels {
		kernel_count = 0,
		nr_kernels = 1,
	} kernel;
} dpu_arguments_t;

typedef struct {
	uint64_t count;
} dpu_results_t;

static bool pred(const uint64_t x)
{
	return (x % 2) == 1;
}
