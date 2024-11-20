#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dpu.h>
#include <dpu_management.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <omp.h>

#include "benchmark.h"
#include "common.h"
#include "database.h"
#include "params.h"
#include "timer.h"

dpu_arguments_t* input_arguments;
dpu_results_t* dpu_results;
struct dpu_set_t dpu_set, dpu;
uint32_t n_dpus;
uint32_t n_ranks;
bool data_on_dpus = false;

unsigned int n_elements_dpu, n_fill_dpu;

double time_alloc, time_load, time_write_data, time_write_command, time_run, time_read_result, time_read_data;
double total_write_data = 0;
double total_write_command = 0;
double total_upmem = 0;
double total_cpu = 0;
double total_read_result = 0;
double total_read_data = 0;

static unsigned int upmem_count(unsigned int n_elements_dpu, enum predicates predicate, uint64_t argument)
{
	unsigned int i = 0;
	startTimer();
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
	time_write_command = stopTimer();
	total_write_command += time_write_command;

	startTimer();
	DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
	time_run = stopTimer();
	total_upmem += time_run;

	startTimer();
	DPU_FOREACH(dpu_set, dpu, i) {
		DPU_ASSERT(dpu_prepare_xfer(dpu, &dpu_results[i]));
	}
	DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, "DPU_RESULTS", 0, sizeof(dpu_results_t), DPU_XFER_DEFAULT));
	unsigned int result_dpu = 0;
	for (i = 0; i < n_dpus; i++) {
		//printf("DPU %4d count == %d\n", i, dpu_results[i].count);
		result_dpu += dpu_results[i].count;
	}
	time_read_result = stopTimer();
	total_read_result += time_read_result;

	printf("[::] COUNT-UPMEM | n_dpus=%d n_ranks=%d n_elements=%d n_elements_per_dpu=%d ",
			n_dpus, n_ranks, n_elements, n_elements_dpu);
	printf("| latency_write_command_us=%f latency_kernel_us=%f latency_read_result_us=%f\n",
			time_write_command, time_run, time_read_result);

	return result_dpu;
}

static void upmem_select(unsigned int n_elements_dpu, enum predicates predicate, uint64_t argument)
{
	unsigned int i = 0;
	startTimer();
	DPU_FOREACH(dpu_set, dpu, i) {
		input_arguments[i].size = n_elements_dpu * sizeof(T);
		input_arguments[i].kernel = kernel_select;
		input_arguments[i].predicate = predicate;
		input_arguments[i].predicate_arg = argument;
		if (i == n_dpus - 1) {
			input_arguments[i].size -= n_fill_dpu * sizeof(T);
		}
		DPU_ASSERT(dpu_prepare_xfer(dpu, &input_arguments[i]));
	}
	DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS", 0, sizeof(dpu_arguments_t), DPU_XFER_DEFAULT));
	time_write_command = stopTimer();

	startTimer();
	DPU_ASSERT(dpu_launch(dpu_set, DPU_SYNCHRONOUS));
	time_run = stopTimer();

	startTimer();
	DPU_FOREACH(dpu_set, dpu, i) {
		DPU_ASSERT(dpu_prepare_xfer(dpu, bitmasks + (n_elements_dpu/32) * i));
	}
	DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 65011712, n_elements_dpu / 32 * sizeof(uint32_t), DPU_XFER_DEFAULT));
	time_read_result = stopTimer();

	printf("[::] SELECT-UPMEM | n_dpus=%d n_ranks=%d n_elements=%d n_elements_per_dpu=%d ",
			n_dpus, n_ranks, n_elements, n_elements_dpu);
	printf("| latency_write_command_us=%f latency_kernel_us=%f latency_read_result_us=%f\n",
			time_write_command, time_run, time_read_result);
}

static void db_to_upmem()
{
	if (data_on_dpus) {
		return;
	}
	unsigned int i = 0;
	startTimer();

	DPU_FOREACH(dpu_set, dpu, i) {
		DPU_ASSERT(dpu_prepare_xfer(dpu, database + n_elements_dpu * i));
	}
	DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_TO_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, n_elements_dpu * sizeof(T), DPU_XFER_DEFAULT));

	data_on_dpus = true;
	time_write_data = stopTimer();
	total_write_data += time_write_data;

	printf("[::] WRITE-UPMEM | n_dpus=%d n_ranks=%d n_elements=%d n_elements_per_dpu=%d ",
			n_dpus, n_ranks, n_elements, n_elements_dpu);
	printf("| latency_write_data_us=%f\n",
			time_write_data);
}

static void db_from_upmem()
{
	if (!data_on_dpus) {
		return;
	}
	unsigned int i = 0;
	startTimer();

	DPU_FOREACH(dpu_set, dpu, i) {
		DPU_ASSERT(dpu_prepare_xfer(dpu, database + n_elements_dpu * i));
	}
	DPU_ASSERT(dpu_push_xfer(dpu_set, DPU_XFER_FROM_DPU, DPU_MRAM_HEAP_POINTER_NAME, 0, n_elements_dpu * sizeof(T), DPU_XFER_DEFAULT));

	data_on_dpus = false;
	time_read_data = stopTimer();
	total_read_data += time_read_data;

	printf("[::] READ-UPMEM | n_dpus=%d n_ranks=%d n_elements=%d n_elements_per_dpu=%d ",
			n_dpus, n_ranks, n_elements, n_elements_dpu);
	printf("| latency_read_data_us=%f\n",
			time_write_data);
}

/*
 * Ensure that each DPU processes a multiple of 64 elements
 * → there are no partially-used bitmasks for SELECT queries at DPU boundaries
 *   (multiple of 32 elements)
 * → all MRAM bitmask accesses are 8-Byte aligned
 *   (multiple of 64 elements == bitmask size is a multiple of 8 Bytes)
 */
static void set_n_elements_dpu(unsigned int n_elem)
{
	n_elements_dpu = n_elem / n_dpus;

	if (n_elem % n_dpus) {
		n_elements_dpu += 1;
	}

	if (n_elements_dpu % 64) {
		n_elements_dpu += 64 - (n_elements_dpu % 64);
	}

	n_fill_dpu = n_elements_dpu * n_dpus - n_elem;

	printf("Using %d elements per DPU (filling DPU %d with %d non-elements)\n", n_elements_dpu, n_dpus -1, n_fill_dpu);
}

int main(int argc, char **argv)
{
	struct Params p;
	unsigned int result_upmem, result_host;

	parse_params(argc, argv, &p);
	n_elements = p.n_elements;

	startTimer();
	DPU_ASSERT(dpu_alloc_ranks(p.n_ranks, NULL, &dpu_set));
	time_alloc = stopTimer();

	startTimer();
	DPU_ASSERT(dpu_load(dpu_set, "bin/dpu_code", NULL));
	time_load = stopTimer();

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

	printf("Allocated %d DPUs across %d ranks for %d elements (%.1f MiB data)\n", n_dpus, n_ranks, p.n_elements, (double)p.n_elements * sizeof(T) / (1024 * 1024));

	set_n_elements_dpu(n_elements);

	database = malloc((p.n_elements + n_fill_dpu) * sizeof(T));
	assert(database != NULL);

	bitmasks = malloc(n_elements / 32 * sizeof(uint32_t) + sizeof(uint32_t));
	assert(bitmasks != NULL);

	input_arguments = malloc(n_dpus * sizeof(dpu_arguments_t));
	assert(input_arguments != NULL);

	dpu_results = malloc(n_dpus * sizeof(dpu_results_t));
	assert(dpu_results != NULL);

	create_db();

	omp_set_num_threads(p.n_threads);

	for (unsigned int i = 0; i < sizeof(benchmark_events) / sizeof(struct benchmark_event); i++) {
		if (benchmark_events[i].op == op_count) {
			db_to_upmem();
			result_upmem = upmem_count(n_elements_dpu, benchmark_events[i].predicate, benchmark_events[i].argument);

			if (p.verify) {
				result_host = host_count(benchmark_events[i].predicate, benchmark_events[i].argument);
				printf("count = %d / %d\n", result_host, result_upmem);
				assert(result_host == result_upmem);
			}

		} else if (benchmark_events[i].op == op_select) {
			db_from_upmem();
			startTimer();
			upmem_select(n_elements_dpu, benchmark_events[i].predicate, benchmark_events[i].argument);
			time_run = stopTimer();

			if (p.verify) {
				result_upmem = count_bits(bitmasks);
				host_select(bitmasks, benchmark_events[i].predicate, benchmark_events[i].argument);
				result_host = count_bits(bitmasks);
				printf("count = %d / %d\n", result_host, result_upmem);
				assert(result_host == result_upmem);
			}

		} else if (benchmark_events[i].op == op_insert) {
			db_from_upmem();
			set_n_elements_dpu(n_elements + benchmark_events[i].argument);
			host_realloc(n_elements + benchmark_events[i].argument + n_fill_dpu);
			startTimer();
			host_insert(benchmark_events[i].argument);
			time_run = stopTimer();
		} else if (benchmark_events[i].op == op_delete) {
			db_from_upmem();
			startTimer();
			result_host = host_delete(benchmark_events[i].predicate, benchmark_events[i].argument);
			time_run = stopTimer();
			printf("delete = %d (%f)\n", result_host, time_run);
			set_n_elements_dpu(n_elements);
			host_realloc(n_elements + n_fill_dpu);
		} else if (benchmark_events[i].op == op_update) {
			db_from_upmem();

			startTimer();
			host_update(benchmark_events[i].predicate,benchmark_events[i].argument);
			time_run = stopTimer();
			total_cpu += time_run;

			printf("[::] UPDATE-CPU | n_elements=%d n_threads=%d ",
					n_elements, p.n_threads);
			printf("| latency_kernel_us=%f\n",
					time_run);

		} else {
			printf("UNSUPPORTED BENCHMARK EVENT %d\n", benchmark_events[i].op);
		}
	}


	free(database);
	free(bitmasks);
	DPU_ASSERT(dpu_free(dpu_set));

	printf("[::] NMCDB | n_elements=%d n_dpus=%d n_ranks=%d n_threads=%d n_elements_per_dpu=%d ",
			p.n_elements, n_dpus, n_ranks, p.n_threads, n_elements_dpu);
	printf("| latency_alloc_us=%f latency_load_us=%f latency_write_data_us=%f latency_write_command_us=%f latency_kernel_upmem_us=%f latency_kernel_cpu=%f latency_read_result_us=%f latency_read_data_us=w%f\n",
			time_alloc, time_load, total_write_data, total_write_command, total_upmem, total_cpu, total_read_result, total_read_data);

	return 0;
}
