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

#include "benchmark_events.h"
