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
#include "params.h"
#include "timer.h"

static T* database;
uint32_t* bitmasks;

static void create_db(T* db, unsigned int n_elements)
{
	for (unsigned int i = 0; i < n_elements; i++) {
		db[i] = i + 1;
	}
}

static unsigned int host_count(T* db, unsigned int n_elements, enum predicates pred, unsigned int pred_arg, unsigned int n_threads)
{
	unsigned int count = 0;
	bool (*_pred_f)(const uint64_t, const uint64_t) = get_pred(pred);
	omp_set_num_threads(n_threads);

	#pragma omp parallel for reduction(+:count)
	for (unsigned int i = 0; i < n_elements; i++) {
		if (_pred_f(db[i], pred_arg)) {
			count++;
		}
	}
	return count;
}

static void host_select(T* db, uint32_t* out, unsigned int n_elements, enum predicates pred, unsigned int pred_arg, unsigned int n_threads)
{
	bool (*_pred_f)(const uint64_t, const uint64_t) = get_pred(pred);
	for (unsigned int i = 0; i < n_elements; i++) {
		if (_pred_f(db[i], pred_arg)) {
			out[i / 32] |= i % 32;
		}
	}
}

int main(int argc, char **argv)
{
	double time;
	struct Params p;

	parse_params(argc, argv, &p);
	assert(p.n_elements % 32 == 0);

	database = malloc(p.n_elements * sizeof(T));
	assert(database != NULL);

	bitmasks = malloc(p.n_elements * sizeof(uint32_t) / 32);
	assert(bitmasks != NULL);

	memset(bitmasks, 0, p.n_elements * sizeof(uint32_t) / 32);
	create_db(database, p.n_elements);

	for (unsigned int i = 0; i < sizeof(benchmark_ops) / sizeof(struct benchmark_op); i++) {
		startTimer();
		unsigned int result_host = host_count(database, p.n_elements, benchmark_ops[i].predicate, benchmark_ops[i].argument, p.n_threads);
		time = stopTimer();
		printf("count = %d (%f)\n", result_host, time);
	}

	free(database);
	free(bitmasks);

	return 0;
}
