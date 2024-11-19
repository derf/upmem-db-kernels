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

enum kernels {
	kernel_count = 0,
	nr_kernels = 1,
};

enum predicates {
	pred_lt = 0,
	pred_le = 1,
	pred_eq = 2,
	pred_ge = 3,
	pred_gt = 4,
	pred_ne = 5,
	pred_bs = 6,
	pred_bc = 7,
	num_predicates = 8,
};

typedef struct {
	uint32_t size;
	uint64_t predicate_arg;
	uint8_t kernel;
	uint8_t predicate;
	uint16_t padding;
} dpu_arguments_t;

typedef struct {
	uint64_t count;
} dpu_results_t;

static bool _pred_lt(const uint64_t x, const uint64_t arg)
{
	return x < arg;
}
static bool _pred_le(const uint64_t x, const uint64_t arg)
{
	return x <= arg;
}
static bool _pred_eq(const uint64_t x, const uint64_t arg)
{
	return x == arg;
}
static bool _pred_ge(const uint64_t x, const uint64_t arg)
{
	return x >= arg;
}
static bool _pred_gt(const uint64_t x, const uint64_t arg)
{
	return x > arg;
}
static bool _pred_ne(const uint64_t x, const uint64_t arg)
{
	return x > arg;
}
static bool _pred_bs(const uint64_t x, const uint64_t arg)
{
	return x & (1<<arg);
}
static bool _pred_bc(const uint64_t x, const uint64_t arg)
{
	return !(x & (1<<arg));
}

bool (*const predicate_functions[num_predicates])(const uint64_t, const uint64_t) = {
	_pred_lt,
	_pred_le,
	_pred_eq,
	_pred_ge,
	_pred_gt,
	_pred_ne,
	_pred_bs,
	_pred_bc
};

bool (*get_pred(enum predicates pred))(const uint64_t, const uint64_t)
{
	return predicate_functions[pred];
}
