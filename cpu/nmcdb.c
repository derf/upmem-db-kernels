#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <stdint.h>
#include <omp.h>
#include <popcntintrin.h>

#include "benchmark.h"
#include "common.h"
#include "params.h"
#include "timer.h"

static T* database;
uint32_t* bitmasks;
unsigned int n_elements;

static void create_db()
{
	for (unsigned int i = 0; i < n_elements; i++) {
		database[i] = i + 1;
	}
}

static unsigned int host_count(enum predicates pred, unsigned int pred_arg)
{
	unsigned int count = 0;
	bool (*_pred_f)(const uint64_t, const uint64_t) = get_pred(pred);

	#pragma omp parallel for reduction(+:count)
	for (unsigned int i = 0; i < n_elements; i++) {
		if (_pred_f(database[i], pred_arg)) {
			count++;
		}
	}
	return count;
}

static void host_insert(unsigned int n_insert)
{
	database = realloc(database, (n_elements + n_insert) * sizeof(T));
	assert(database != NULL);

	bitmasks = realloc(bitmasks, (n_elements + n_insert) / 32 * sizeof(uint32_t) + sizeof(uint32_t));
	assert(bitmasks != NULL);

	for (unsigned int i = n_elements; i < n_elements + n_insert; i++) {
		database[i] = i + 1;
	}
	n_elements += n_insert;
}

static unsigned int host_update(enum predicates pred, unsigned int pred_arg)
{
	unsigned int count = 0;
	bool (*_pred_f)(const uint64_t, const uint64_t) = get_pred(pred);

	#pragma omp parallel for reduction(+:count)
	for (unsigned int i = 0; i < n_elements; i++) {
		if (_pred_f(database[i], pred_arg)) {
			database[i] = pred_arg;
			count++;
		}
	}
	return count;
}

static unsigned int host_delete(enum predicates pred, unsigned int pred_arg)
{
	bool (*_pred_f)(const uint64_t, const uint64_t) = get_pred(pred);
	unsigned int n_delete = 0;
	for (unsigned int i = 0; i < n_elements; i++) {
		if (_pred_f(database[i], pred_arg)) {
			n_delete += 1;
		} else if (n_delete) {
			database[i-n_delete] = database[i];
		}
	}

	database = realloc(database, (n_elements - n_delete) * sizeof(T));
	assert(database != NULL);

	bitmasks = realloc(bitmasks, (n_elements - n_delete) / 32 * sizeof(uint32_t) + sizeof(uint32_t));
	assert(bitmasks != NULL);

	n_elements -= n_delete;
	return n_delete;
}

static void host_select(uint32_t* out, enum predicates pred, unsigned int pred_arg)
{
	bool (*_pred_f)(const uint64_t, const uint64_t) = get_pred(pred);
	memset(bitmasks, 0, n_elements * sizeof(uint32_t) / 32 + sizeof(uint32_t));

	#pragma omp parallel for
	for (unsigned int i = 0; i < n_elements/32; i++) {
		for (unsigned int j = 0; j < 32; j++) {
			out[i] |= (_pred_f(database[i*32+j], pred_arg) * 1) << j;
		}
	}

	for (unsigned int j = 0; j < n_elements % 32; j++) {
		out[n_elements/32] |= (_pred_f(database[(n_elements/32)*32 + j], pred_arg) * 1) << j;
	}
}

static unsigned int count_bits(uint32_t* masks)
{
	unsigned int count = 0;

	#pragma omp parallel for reduction(+:count)
	for (unsigned int i = 0; i < n_elements/32; i++) {
		count += _mm_popcnt_u32(masks[i]);
	}

	for (unsigned int j = 0; j < n_elements % 32; j++) {
		if (masks[n_elements/32] & (1<<j)) {
			count += 1;
		}
	}

	return count;
}

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
