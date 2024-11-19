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
};
