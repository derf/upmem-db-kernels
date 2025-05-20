#!/bin/bash

/opt/upmem/upmem-2025.1.0-Linux-x86_64/upmem_env.sh

run_benchmark_nmc()
{
	local "$@"
	util/generate-benchmark.py --operation=${operation} --n-operations=${n_operations} > include/benchmark_events.h
	make -B tinos=1 aspectc=1 dfatool_timing=0 aspectc_timing=1 2> /dev/null
	numactl -m ${numa_mem} -C ${numa_cores} bin/host_code -i ${n_elements} -n ${n_threads} -r ${n_ranks}
	if [ "$n_operations" = 1 ]; then
		util/generate-benchmark.py --n-operations=100 --with-write > include/benchmark_events.h
		make -B tinos=1 aspectc=1 dfatool_timing=0 aspectc_timing=1 2> /dev/null
		numactl -m ${numa_mem} -C ${numa_cores} bin/host_code -i ${n_elements} -n ${n_threads} -r ${n_ranks}
	fi
}

export -f run_benchmark_nmc

mkdir -p log/$(hostname)

fn=log/$(hostname)/ccmcc25

parallel -j1 --eta --joblog ${fn}.node1.joblog --resume --header : \
	run_benchmark_nmc numa_mem={numa_mem} numa_cores={numa_cores} n_elements={n_elements} n_threads={n_threads} n_ranks={n_ranks} operation={operation} n_operations={n_operations} \
	::: numa_mem 1 \
	::: numa_cores 8-15 \
	::: operation count select \
	::: n_operations 1 2 4 8 12 16 20 \
	::: n_elements $((1024*1024)) $((1024*1024*2)) $((1024*1024*4)) $((1024*1024*8)) $((1024*1024*16)) $((1024*1024*32)) $((1024*1024*64)) $((1024*1024*128)) $((1024*1024*256)) $((1024*1024*512)) $((1024*1024*1024)) $((1024*1024*1024*2)) $((1024*1024*1024*4)) \
	::: n_threads 8 \
	::: n_ranks 1 2 4 8 12 16 20 24 28 32 36 40 \
	::: i 1 2 3 4 5 >> ${fn}.node1.txt \
>> ${fn}.node1.txt

git checkout include/benchmark_events.h
