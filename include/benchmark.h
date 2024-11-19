#include "common.h"

struct benchmark_op {
	enum predicates predicate;
	uint64_t argument;
};

struct benchmark_op benchmark_ops[] = {
	{pred_le, 123},
	{pred_gt, 65535},
	{pred_bs, 5},
	{pred_bc, 7},
	{pred_eq, 423},
	{pred_ne, 7777},
	{pred_le, 12377},
	{pred_gt, 6535},
	{pred_bs, 4},
	{pred_bc, 0},
	{pred_eq, 42753},
	{pred_ne, 77277},
	{pred_le, 1},
	{pred_gt, 767547654},
	{pred_bs, 4},
	{pred_bc, 0},
	{pred_eq, 2754275},
	{pred_ne, 0},
};
