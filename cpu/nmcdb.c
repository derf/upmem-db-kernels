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
	double total_time = 0;
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
			host_count(benchmark_events[i].predicate, benchmark_events[i].argument);
			time = stopTimer();
			total_time += time;

			printf("[::] COUNT-CPU | n_elements=%lu n_threads=%d ",
					n_elements, p.n_threads);
			printf("| latency_kernel_us=%f\n",
					time);;
		} else if (benchmark_events[i].op == op_select) {
			startTimer();
			host_select(bitmasks, benchmark_events[i].predicate, benchmark_events[i].argument);
			time = stopTimer();
			total_time += time;

			printf("[::] SELECT-CPU | n_elements=%lu n_threads=%d ",
					n_elements, p.n_threads);
			printf("| latency_kernel_us=%f\n",
					time);
		} else if (benchmark_events[i].op == op_insert) {
			host_realloc(n_elements + benchmark_events[i].argument);

			startTimer();
			host_insert(benchmark_events[i].argument);
			time = stopTimer();
			total_time += time;

			printf("[::] INSERT-CPU | n_elements=%lu n_threads=%d ",
					n_elements, p.n_threads);
			printf("| latency_kernel_us=%f\n",
					time);
		} else if (benchmark_events[i].op == op_delete) {
			startTimer();
			host_delete(benchmark_events[i].predicate, benchmark_events[i].argument);
			time = stopTimer();
			total_time += time;

			host_realloc(n_elements);

			printf("[::] DELETE-CPU | n_elements=%lu n_threads=%d ",
					n_elements, p.n_threads);
			printf("| latency_kernel_us=%f\n",
					time);
		} else if (benchmark_events[i].op == op_update) {
			startTimer();
			unsigned int count = host_update(benchmark_events[i].predicate,benchmark_events[i].argument);
			time = stopTimer();
			total_time += time;

			printf("[::] UPDATE-CPU | n_elements=%lu n_threads=%d ",
					n_elements, p.n_threads);
			printf("| latency_kernel_us=%f\n",
					time);
		} else {
			printf("UNSUPPORTED BENCHMARK EVENT %d\n", benchmark_events[i].op);
		}
	}

	free(database);
	free(bitmasks);

	printf("[::] NMCDB-CPU | n_elements=%lu n_threads=%d ",
			p.n_elements, p.n_threads);
	printf("| latency_kernel_cpu=%f latency_post_setup_us=%f latency_total_us=%f\n",
			total_time, total_time, total_time);

	return 0;
}
