#!/bin/bash
# Final benchmark of nonosolve on the server, pinned to two CPUs.
# usage: final_ours.sh <cpuA> <cpuB>
ROOT=$(cd "$(dirname "$0")/.." && pwd)
A=$1; B=$2
cd $ROOT
R=bench/results
# 1. webpbn survey sample set + hardest webpbn puzzles (uniqueness check, 1 h limit)
python3 bench/run_bench.py --solvers ours --timeout 3600 --jobs 2 --cpus $A,$B --out $R/survey_ours.csv \
  puzzles/survey/*.nin puzzles/hard/*.nin > $R/survey_ours.log 2>&1
# 2. all tournament sets (first solution, in order), split over the two CPUs
T=$(ls puzzles/tournament/*.txt)
SA=$(echo $T | tr " " "\n" | sed -n "1~2p" | tr "\n" " ")
SB=$(echo $T | tr " " "\n" | sed -n "2~2p" | tr "\n" " ")
rm -rf $R/ours_final
bench/run_ours_sets.sh $A final $SA &
bench/run_ours_sets.sh $B final $SB &
wait
# 3. Wolter's 5000 random 30x30 puzzles (uniqueness check, 120 s limit)
python3 bench/run_bench.py --solvers ours --timeout 120 --jobs 2 --cpus $A,$B --out $R/rand30_ours.csv \
  puzzles/rand30/*.nin > $R/rand30_ours.log 2>&1
echo FINAL_DONE > $R/final_ours.done
