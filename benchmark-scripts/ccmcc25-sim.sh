#!/bin/bash

source ~/lib/local/upmem/upmem-2025.1.0-Linux-x86_64/upmem_env.sh simulator

run_benchmark_nmc()
{
	local "$@"
	util/generate-benchmark.py --operation=${operation} --n-operations=${n_operations} > include/benchmark_events.h
	make -B chios=1 aspectc=1 dfatool_timing=0 aspectc_timing=1 2> /dev/null
	bin/host_code -i ${n_elements} -n ${n_threads} -r ${n_ranks}
	if [ "$n_operations" = 1 ]; then
		util/generate-benchmark.py --n-operations=100 --with-add --with-write > include/benchmark_events.h
		make -B chios=1 aspectc=1 dfatool_timing=0 aspectc_timing=1 2> /dev/null
		bin/host_code -i ${n_elements} -n ${n_threads} -r ${n_ranks}
	fi
}

export -f run_benchmark_nmc

mkdir -p log/$(hostname)

fn=log/$(hostname)/ccmcc25

echo "nodmc-upmem-db  $(git describe --all --long)  $(git rev-parse HEAD)  $(date -R)" >> ${fn}.txt

parallel -j1 --eta --joblog ${fn}.joblog --resume --header : \
	run_benchmark_nmc n_elements={n_elements} n_threads={n_threads} n_ranks={n_ranks} operation={operation} n_operations={n_operations} \
	::: operation count select \
	::: n_operations 1 10 20 \
	::: n_elements $((1024*512)) $((1024*1024)) $((1024*1024*2)) $((1024*1024*4)) \
	::: n_threads 8 \
	::: n_ranks 1 2 4 8 12 16 20 24 28 32 36 40 \
>> ${fn}.txt

git checkout include/benchmark_events.h
