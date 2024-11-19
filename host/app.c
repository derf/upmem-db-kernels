#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dpu.h>
#include <dpu_management.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>

#include "benchmark.h"
#include "common.h"
#include "params.h"

static T* database;
uint32_t* bitmasks;

dpu_arguments_t* input_arguments;
dpu_results_t* dpu_results;
struct dpu_set_t dpu_set, dpu;
uint32_t n_dpus;
uint32_t n_ranks;

unsigned int n_elements_dpu, n_fill_dpu;

static void create_db(T* db, unsigned int nr_elements)
{
	for (unsigned int i = 0; i < nr_elements; i++) {
		db[i] = i + 1;
	}
}

static unsigned int host_count(T* db, unsigned int nr_elements, enum predicates pred, uint64_t pred_arg)
{
	unsigned int count = 0;
	bool (*_pred_f)(const uint64_t, const uint64_t) = get_pred(pred);
	for (unsigned int i = 0; i < nr_elements; i++) {
		if (_pred_f(db[i], pred_arg)) {
			count++;
		}
	}
	return count;
}

static void host_select(T* db, uint32_t* out, unsigned int nr_elements, enum predicates pred, uint64_t pred_arg)
{
	bool (*_pred_f)(const uint64_t, const uint64_t) = get_pred(pred);
	for (unsigned int i = 0; i < nr_elements; i++) {
		if (_pred_f(db[i], pred_arg)) {
			out[i / 32] |= i % 32;
		}
	}
}

unsigned int upmem_count(unsigned int n_elements_dpu, enum predicates predicate, uint64_t argument)
{
	unsigned int i = 0;
	DPU_FOREACH(dpu_set, dpu, i) {
		input_arguments[i].size = n_elements_dpu * sizeof(T);
		input_arguments[i].kernel = kernel_count;
		input_arguments[i].predicate = predicate;
		input_arguments[i].predicate_arg = argument;
		if (i == n_dpus - 1) {
			input_arguments[i].size -= n_fill_dpu * sizeof(T);
		}
		DPU_ASSERT(dpu_prepare_xfer(dpu, &input_arguments[i]));
	}
	DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));
	DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
	DPU_FOREACH(dpu_set, dpu, i) {
		DPU_ASSERT(dpu_prepare_xfer(dpu, &dpu_results[i]));
	}
	DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, "DPU_RESULTS", 0, sizeof(dpu_results_t), DPU_XFER_DEFAULT));
	unsigned int result_dpu = 0;
	for (i = 0; i < n_dpus; i++) {
		result_dpu += dpu_results[i].count;
	}
	return result_dpu;
}

int main(int argc, char **argv)
{
	struct Params p;
	unsigned int i = 0;
	unsigned int result_upmem, result_host;

	parse_params(argc, argv, &p);

	DPU_ASSERT(dpu_alloc_ranks(p.n_ranks, NULL, &dpu_set));
	DPU_ASSERT(dpu_load(dpu_set, "bin/dpu_code", NULL));
	DPU_ASSERT(dpu_get_nr_dpus(dpu_set, &n_dpus));
	DPU_ASSERT(dpu_get_nr_ranks(dpu_set, &n_ranks));

	/*
	 * 64 MiB total per DPU == 8388608 elements
	 * - ~16B housekeeping (~16B)
	 * - 1 MiB output (worst case: 262144 32-bit bitmasks for SELECT output)
	 * - a few KiB input (INSERT, UPDATE, DELETE)
	 * == 62 MiB total for input data
	 * == 8126464 elements per DPU
	 */
	assert(n_dpus * 8126464 > p.n_elements);

	assert(p.n_elements % 32 == 0);

	n_elements_dpu = p.n_elements / n_dpus;
	n_fill_dpu = 0;
	if (p.n_elements % n_dpus) {
		n_elements_dpu += 1;
		n_fill_dpu = n_elements_dpu * n_dpus - p.n_elements;
	}

	printf("Allocated %d DPUs across %d ranks for %d elements (%.1f MiB data)\n", n_dpus, n_ranks, p.n_elements, (double)p.n_elements * sizeof(T) / (1024 * 1024));
	printf("Using %d elements per DPU\n", n_elements_dpu);
	printf("DPU %d is filled with %d non-elements for even distribution\n", n_dpus - 1, n_fill_dpu);

	database = malloc(p.n_elements * sizeof(T));
	assert(database != NULL);

	bitmasks = malloc(p.n_elements * sizeof(uint32_t) / 32);
	assert(bitmasks != NULL);

	input_arguments = malloc(n_dpus * sizeof(dpu_arguments_t));
	assert(input_arguments != NULL);

	dpu_results = malloc(n_dpus * sizeof(dpu_results_t));
	assert(dpu_results != NULL);

	memset(bitmasks, 0, p.n_elements * sizeof(uint32_t) / 32);
	create_db(database, p.n_elements);

	DPU_FOREACH(dpu_set, dpu, i) {
		DPU_ASSERT(dpu_prepare_xfer(dpu, database + n_elements_dpu * i));
	}
	DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, n_elements_dpu * sizeof(T), DPU_XFER_DEFAULT));

	for (unsigned int i = 0; i < sizeof(benchmark_ops) / sizeof(struct benchmark_op); i++) {
		result_upmem = upmem_count(n_elements_dpu, benchmark_ops[i].predicate, benchmark_ops[i].argument);
		result_host = host_count(database, p.n_elements, benchmark_ops[i].predicate, benchmark_ops[i].argument);
		printf("count = %d / %d\n", result_host, result_upmem);
		assert(result_host == result_upmem);
	}


	free(database);
	free(bitmasks);
	DPU_ASSERT(dpu_free(dpu_set));

	return 0;
}
