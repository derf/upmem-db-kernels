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
	kernel_select = 1,
	kernel_update = 2,
	nr_kernels = 3,
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

static char* const predicate_names[] = {
	"≤",
	"<",
	"==",
	"≥",
	">",
	"≠",
	"bit",
	"!bit"
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

static bool _pred_lt(uint64_t const x, uint64_t const arg)
{
	return x < arg;
}
static bool _pred_le(uint64_t const x, uint64_t const arg)
{
	return x <= arg;
}
static bool _pred_eq(uint64_t const x, uint64_t const arg)
{
	return x == arg;
}
static bool _pred_ge(uint64_t const x, uint64_t const arg)
{
	return x >= arg;
}
static bool _pred_gt(uint64_t const x, uint64_t const arg)
{
	return x > arg;
}
static bool _pred_ne(uint64_t const x, uint64_t const arg)
{
	return x > arg;
}
static bool _pred_bs(uint64_t const x, uint64_t const arg)
{
	return x & (1<<arg);
}
static bool _pred_bc(uint64_t const x, uint64_t const arg)
{
	return !(x & (1<<arg));
}

bool (*const predicate_functions[num_predicates])(uint64_t const, uint64_t const) = {
	_pred_lt,
	_pred_le,
	_pred_eq,
	_pred_ge,
	_pred_gt,
	_pred_ne,
	_pred_bs,
	_pred_bc
};

bool (*get_pred(enum predicates pred))(uint64_t const, uint64_t const)
{
	return predicate_functions[pred];
}
