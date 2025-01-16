#!/bin/sh

op_filter="$1"
n_elem="$2"
n_threads="$3"
n_ranks="$4"

echo CPU:
fgrep "NMCDB-CPU | n_elements=${n_elem}" log/tinos/validation.txt | fgrep "n_${op_filter} " | fgrep "n_threads=${n_threads} " | grep -Eo 'latency_total_us=[^ ]*'

echo
echo UPMEM:
fgrep "NMCDB-UPMEM | n_elements=${n_elem}" log/tinos/validation.txt | fgrep "n_${op_filter} " | fgrep "n_threads=1 " | fgrep "n_ranks=${n_ranks} " | grep -Eo 'latency_total_us=[^ ]*'
