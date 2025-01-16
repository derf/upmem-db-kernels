#!/bin/bash

source /opt/upmem/upmem-2024.2.0-Linux-x86_64/upmem_env.sh
git checkout include/benchmark_events.h
make -B tinos=1

mkdir -p log/$(hostname)

run_benchmark_cpu()
{
	local "$@"
	numactl -m ${numa_mem} -C ${numa_cores} bin/cpu_code -i ${n_elements} -n ${n_threads}
}

run_benchmark_nmc()
{
	local "$@"
	numactl -m ${numa_mem} -C ${numa_cores} bin/host_code -i ${n_elements} -n ${n_threads} -r ${n_ranks}
}

export -f run_benchmark_cpu
export -f run_benchmark_nmc

fn=log/$(hostname)/nodmc25-cpu

parallel -j1 --eta --joblog ${fn}.node1.joblog --resume --header : \
	run_benchmark_cpu numa_mem={numa_mem} numa_cores={numa_cores} n_elements={n_elements} n_threads={n_threads} \
	::: numa_mem 1 \
	::: numa_cores 8-15 \
	::: n_elements $((1024*1024)) $((1024*1024*2)) $((1024*1024*4)) $((1024*1024*8)) $((1024*1024*16)) $((1024*1024*32)) $((1024*1024*64)) $((1024*1024*128)) $((1024*1024*256)) $((1024*1024*512)) $((1024*1024*1024)) $((1024*1024*1024*2)) $((1024*1024*1024*3)) $((1024*1024*1024*4)) $((1024*1024*1024*5)) $((1024*1024*1024*6)) $((1024*1024*1024*7)) \
	::: n_threads 1 2 4 6 8 \
	::: i 1 2 3 4 5 >> ${fn}.node1.txt

fn=log/$(hostname)/nodmc25-upmem

parallel -j1 --eta --joblog ${fn}.node1.joblog --resume --header : \
	run_benchmark_nmc numa_mem={numa_mem} numa_cores={numa_cores} n_elements={n_elements} n_threads={n_threads} n_ranks={n_ranks} \
	::: numa_mem 1 \
	::: numa_cores 8-15 \
	::: n_elements $((1024*1024)) $((1024*1024*2)) $((1024*1024*4)) $((1024*1024*8)) $((1024*1024*16)) $((1024*1024*32)) $((1024*1024*64)) $((1024*1024*128)) $((1024*1024*256)) $((1024*1024*512)) $((1024*1024*1024)) $((1024*1024*1024*2)) $((1024*1024*1024*3)) $((1024*1024*1024*4)) $((1024*1024*1024*5)) $((1024*1024*1024*6)) $((1024*1024*1024*7)) \
	::: n_threads 1 2 4 6 8 \
	::: n_ranks 1 2 4 8 12 16 20 24 28 32 36 40 \
	::: i 1 2 3 4 5 >> ${fn}.node1.txt

fn=log/$(hostname)/nodmc25-cpu

parallel -j1 --eta --joblog ${fn}.node0.joblog --resume --header : \
	run_benchmark_cpu numa_mem={numa_mem} numa_cores={numa_cores} n_elements={n_elements} n_threads={n_threads} \
	::: numa_mem 0 \
	::: numa_cores 0-7 \
	::: n_elements $((1024*1024)) $((1024*1024*2)) $((1024*1024*4)) $((1024*1024*8)) $((1024*1024*16)) $((1024*1024*32)) $((1024*1024*64)) $((1024*1024*128)) $((1024*1024*256)) $((1024*1024*512)) $((1024*1024*1024)) $((1024*1024*1024*2)) $((1024*1024*1024*3)) $((1024*1024*1024*4)) $((1024*1024*1024*5)) $((1024*1024*1024*6)) \
	::: n_threads 1 2 4 6 8 \
	::: i 1 2 3 4 5 >> ${fn}.node0.txt

fn=log/$(hostname)/nodmc25-upmem

parallel -j1 --eta --joblog ${fn}.node0.joblog --resume --header : \
	run_benchmark_nmc numa_mem={numa_mem} numa_cores={numa_cores} n_elements={n_elements} n_threads={n_threads} n_ranks={n_ranks} \
	::: numa_mem 0 \
	::: numa_cores 0-7 \
	::: n_elements $((1024*1024)) $((1024*1024*2)) $((1024*1024*4)) $((1024*1024*8)) $((1024*1024*16)) $((1024*1024*32)) $((1024*1024*64)) $((1024*1024*128)) $((1024*1024*256)) $((1024*1024*512)) $((1024*1024*1024)) $((1024*1024*1024*2)) $((1024*1024*1024*3)) $((1024*1024*1024*4)) $((1024*1024*1024*5)) $((1024*1024*1024*6)) \
	::: n_threads 1 2 4 6 8 \
	::: n_ranks 1 2 4 8 12 16 20 24 28 32 36 40 \
	::: i 1 2 3 4 5 >> ${fn}.node0.txt
