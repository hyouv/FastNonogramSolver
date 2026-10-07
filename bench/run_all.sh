#!/bin/bash
# The full evaluation behind results/ (uses CPUs 0-7 only; one job per core).
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd $ROOT
R=bench/results
mkdir -p $R
# competitors on the webpbn puzzles: CPUs 0-3
python3 bench/run_bench.py --solvers pbnsolve,naughty,jsolver,grid,nonogrid_bt,nonogrid_sat --timeout 600 \
  --jobs 4 --cpus 0,1,2,3 --out $R/survey_competitors.csv puzzles/survey/*.nin puzzles/hard/*.nin \
  > $R/survey_competitors.log 2>&1 &
COMP=$!
# LalaFrogKK on the tournament sets: CPUs 4-5, plus 0-3 once the competitor run is done
bench/run_lala_queue.sh 4 > /dev/null 2>&1 &
bench/run_lala_queue.sh 5 > /dev/null 2>&1 &
# nonosolve on everything: CPUs 6-7
bench/final_ours.sh 6 7 > $R/final_ours.out 2>&1 &
wait $COMP
for c in 0 1 2 3; do bench/run_lala_queue.sh $c > /dev/null 2>&1 & done
wait
python3 bench/export_results.py
