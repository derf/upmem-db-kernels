#!/bin/bash

set -ex

source /opt/upmem/upmem-2024.2.0-Linux-x86_64/upmem_env.sh

: > log/$(hostname)/validation.txt

for n_operations in $(seq 1 5); do
	for operation in count select; do

		util/generate-benchmark.py --operation=${operation} --n-operations=${n_operations} > include/benchmark_events.h
		make -B tinos=1

		for n_elements in $((2**25)) $((2**26)) $((2**27)) $((2**28)) $((2**29)); do
			for i in $(seq 1 5); do
				numactl -m 1 -C 8-15 bin/cpu_code -i ${n_elements} -n 1 >> log/$(hostname)/validation.txt || true
			done
			for i in $(seq 1 5); do
				numactl -m 1 -C 8-15 bin/cpu_code -i ${n_elements} -n 4 >> log/$(hostname)/validation.txt || true
			done
			for n_ranks in $(seq 1 40); do
				for i in $(seq 1 5); do
					numactl -m 1 -C 8-15 bin/host_code -i ${n_elements} -n 1 -r ${n_ranks} >> log/$(hostname)/validation.txt || true
				done
			done
		done

	done
done
