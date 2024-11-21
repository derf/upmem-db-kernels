#include "common.h"

enum benchmark_statements {
	op_count = 0,
	op_select = 1,
	op_insert = 2,
	op_update = 3,
	op_delete = 4,
	nr_benchmark_statements = 5
};

struct benchmark_event {
	enum benchmark_statements op;
	enum predicates predicate;
	uint64_t argument;
};

struct benchmark_event benchmark_events[] = {
	{op_count, pred_le, 123},
	{op_select, pred_le, 123},
	{op_count, pred_gt, 65535},
	{op_select, pred_gt, 65535},
	{op_count, pred_bs, 5},
	{op_select, pred_bs, 5},
	{op_count, pred_bc, 7},
	{op_select, pred_bc, 7},
	{op_count, pred_eq, 423},
	{op_select, pred_eq, 423},
	{op_count, pred_ne, 7777},
	{op_select, pred_ne, 7777},
	{op_count, pred_le, 12377},
	{op_count, pred_gt, 6535},
	{op_count, pred_bs, 4},
	{op_count, pred_bc, 0},
	{op_count, pred_eq, 42753},
	{op_count, pred_ne, 77277},
	{op_update, pred_eq, 7777},
	{op_count, pred_le, 1},
	{op_count, pred_gt, 767547654},
	{op_count, pred_bs, 4},
	{op_count, pred_bc, 0},
	{op_count, pred_eq, 2754275},
	{op_count, pred_ne, 0},
	{op_count, pred_eq, 423},
	{op_select, pred_eq, 423},
	{op_count, pred_ne, 7777},
	{op_select, pred_ne, 7777},
	{op_count, pred_le, 12377},
	{op_select, pred_le, 12377},
	{op_count, pred_gt, 6535},
	{op_select, pred_gt, 6535},
	{op_count, pred_ge, 0},
	{op_select, pred_ge, 0},
	{op_update, pred_ne, 7778},
	{op_count, pred_eq, 7787},
	{op_select, pred_eq, 8777},
	{op_count, pred_bs, 4},
	{op_count, pred_bc, 0},
	{op_count, pred_eq, 2754275},
	{op_count, pred_ne, 0},
	{op_count, pred_eq, 423},
	{op_select, pred_eq, 423},
	{op_count, pred_ne, 7777},
	{op_select, pred_ne, 7777},
	{op_count, pred_le, 12377},
	{op_select, pred_le, 12377},
	{op_count, pred_gt, 6535},
	{op_select, pred_gt, 6535},
};
