#!/bin/bash
# Run nonosolve on tournament sets, pinned to one CPU.
# usage: run_ours_sets.sh <cpu> <tag> <set.txt>... (extra solver flags via $FLAGS)
ROOT=$(cd "$(dirname "$0")/.." && pwd)
CPU=$1; TAG=$2; shift 2
d=$ROOT/bench/results/ours_$TAG
mkdir -p $d
for f0 in "$@"; do
  f=$(realpath $f0)
  name=$(basename $f .txt)
  /usr/bin/time -f "%U %S %e" -o $d/$name.time taskset -c $CPU $ROOT/bin/nonosolve $FLAGS --taai $f -o $d/$name.sol --log $d/$name.log > $d/$name.out 2>&1
  echo "$name $(cat $d/$name.out) $(tail -1 $d/$name.log) $(cat $d/$name.time)" >> $d/summary.txt
done
