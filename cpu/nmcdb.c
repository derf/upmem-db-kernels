#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <assert.h>
#include <stdint.h>
#include <omp.h>

#include "common.h"
#include "params.h"

static T* database;
uint32_t* bitmasks;

static void create_db(T* db, unsigned int n_elements)
{
	for (unsigned int i = 0; i < n_elements; i++) {
		db[i] = i + 1;
	}
}

static unsigned int host_count(T* db, unsigned int n_elements, unsigned int n_threads)
{
	unsigned int count = 0;
	omp_set_num_threads(n_threads);

	#pragma omp parallel for reduction(+:count)
	for (unsigned int i = 0; i < n_elements; i++) {
		if (pred(db[i])) {
			count++;
		}
	}
	return count;
}

static void host_select(T* db, uint32_t* out, unsigned int n_elements, unsigned int n_threads)
{
	for (unsigned int i = 0; i < n_elements; i++) {
		if (pred(db[i])) {
			out[i / 32] |= i % 32;
		}
	}
}

int main(int argc, char **argv)
{
	struct Params p;

	parse_params(argc, argv, &p);
	assert(p.n_elements % 32 == 0);

	database = malloc(p.n_elements * sizeof(T));
	assert(database != NULL);

	bitmasks = malloc(p.n_elements * sizeof(uint32_t) / 32);
	assert(bitmasks != NULL);

	memset(bitmasks, 0, p.n_elements * sizeof(uint32_t) / 32);
	create_db(database, p.n_elements);

	unsigned int result_host = host_count(database, p.n_elements, p.n_threads);
	host_select(database, bitmasks, p.n_elements, p.n_threads);
	printf("count = %d\n", result_host);

	free(database);
	free(bitmasks);

	return 0;
}
