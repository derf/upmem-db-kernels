#pragma once

#if NUMA
#include <numaif.h>
#include <numa.h>
#endif

typedef struct Params {
	unsigned long  n_elements;
	unsigned int   n_threads;
	unsigned int   n_ranks;
	bool           verify;
#if NUMA
	struct bitmask* bitmask_data;
	int numa_node_cpu;
#endif
}Params;

void parse_params(int argc, char **argv, struct Params *p) {
	p->n_elements    = 8 << 10;
	p->n_ranks       = 20;
	p->n_threads     = 4;
	p->verify        = false;
#if NUMA
	p->bitmask_data   = NULL;
	p->numa_node_cpu = -1;
#endif

	int opt;
	while((opt = getopt(argc, argv, "i:n:r:A:B:C:V")) >= 0) {
		switch(opt) {
			case 'i': p->n_elements    = atol(optarg); break;
			case 'n': p->n_threads     = atoi(optarg); break;
			case 'r': p->n_ranks       = atoi(optarg); break;
			case 'V': p->verify        = true; break;
#if NUMA
			case 'C': p->numa_node_cpu = atoi(optarg); break;
			case 'D': p->bitmask_data  = numa_parse_nodestring(optarg); break;
#endif
			default:
				fprintf(stderr, "\nUnrecognized option!\n");
				exit(1);
		}
	}
}
