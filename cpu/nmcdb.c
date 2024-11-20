#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <stdint.h>
#include <omp.h>

#include "benchmark.h"
#include "common.h"
#include "database.h"
#include "params.h"
#include "timer.h"

int main(int argc, char **argv)
{
	double time;
	struct Params p;

	parse_params(argc, argv, &p);
	n_elements = p.n_elements;

	database = malloc(n_elements * sizeof(T));
	assert(database != NULL);

	bitmasks = malloc(n_elements / 32 * sizeof(uint32_t) + sizeof(uint32_t));
	assert(bitmasks != NULL);

	create_db();

	omp_set_num_threads(p.n_threads);

	for (unsigned int i = 0; i < sizeof(benchmark_events) / sizeof(struct benchmark_event); i++) {
		if (benchmark_events[i].op == op_count) {
			startTimer();
			unsigned int result_host = host_count(benchmark_events[i].predicate, benchmark_events[i].argument);
			time = stopTimer();
			printf("count = %d (%f)\n", result_host, time);
		} else if (benchmark_events[i].op == op_select) {
			startTimer();
			host_select(bitmasks, benchmark_events[i].predicate, benchmark_events[i].argument);
			time = stopTimer();
			printf("select = %d (%f)\n", count_bits(bitmasks), time);
		} else if (benchmark_events[i].op == op_insert) {
			startTimer();
			host_insert(benchmark_events[i].argument);
			time = stopTimer();
			printf("insert = %lu (%f)\n", benchmark_events[i].argument, time);
		} else if (benchmark_events[i].op == op_delete) {
			startTimer();
			unsigned int count = host_delete(benchmark_events[i].predicate, benchmark_events[i].argument);
			time = stopTimer();
			printf("delete = %d (%f)\n", count, time);
		} else if (benchmark_events[i].op == op_update) {
			startTimer();
			unsigned int count = host_update(benchmark_events[i].predicate,benchmark_events[i].argument);
			time = stopTimer();
			printf("update = %d (%f)\n", count, time);
		} else {
			printf("UNSUPPORTED BENCHMARK EVENT %d\n", benchmark_events[i].op);
		}
	}

	free(database);
	free(bitmasks);

	return 0;
}
