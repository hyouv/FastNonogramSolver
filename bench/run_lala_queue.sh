#!/bin/bash
# Worker: claims tournament sets one at a time (mkdir lock) and runs LalaFrogKK
# on them pinned to <cpu>, with the tournament limit of 2 hours per set.
ROOT=$(cd "$(dirname "$0")/.." && pwd)
CPU=$1
cd $ROOT
mkdir -p bench/results/lala_claims bench/results/lala
for f in $ROOT/puzzles/tournament/*.txt; do
  name=$(basename "$f" .txt)
  mkdir bench/results/lala_claims/$name 2>/dev/null || continue
  d=$ROOT/bench/results/lala/$name
  rm -rf "$d"; mkdir -p "$d"
  (cd "$d" && cp "$f" input.txt &&
   /usr/bin/time -f "%U %S %e" -o time.txt timeout 7200 taskset -c $CPU $ROOT/third_party/CGI-LAB_Nonogram/lalafrog > /dev/null 2>&1)
  echo "$name cpu$CPU solved=$(grep -c '^#' $d/log.txt) $(grep 'total time' $d/log.txt) time=$(tail -1 $d/time.txt)" >> bench/results/lala/summary.txt
done
