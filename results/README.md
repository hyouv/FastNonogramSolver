# Benchmark results

All numbers were measured on one server (2x Intel Xeon Gold 5416S, Ubuntu 20.04, gcc 9.4), one process per physical core pinned with `taskset`, at most 8 jobs at a time.  Times are CPU seconds (user + system).

* **webpbn puzzles**: find up to two solutions, i.e. solve and check uniqueness.  Competitors had 600 s; nonosolve had 3600 s, and only #25820 (632 s) needed more than 600 s.  `n/a` = puzzle too large for the solver, `err` = the solver rejected the input.
* **Tournament sets** (TAAI/TCGA/ICGA, 1000 puzzles of 25x25 each): first solution of every puzzle, in order, with the tournament limit of 2 hours per set.
* **Random 30x30 puzzles** (Wolter): solve and check uniqueness, 120 s limit.

Files: `webpbn.csv`, `webpbn_seeds.csv`, `tournament_sets.csv`, `tournament_puzzles.csv` (per-puzzle times), `rand30.csv`.

## webpbn puzzles

### Wolter survey sample set

| puzzle | nonosolve | pbnsolve | naughty | jsolver | grid | nonogrid_bt | nonogrid_sat |
|---|---:|---:|---:|---:|---:|---:|---:|
| Dancer | 0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
| Cat | 0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
| Skid | 0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
| Bucks | 0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
| Edge | 0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
| Smoke | 0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
| Knot | 0.03 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
| Swing | 0.03 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
| Mum | 0.03 | 0.01 | 0.01 | <0.01 | <0.01 | 0.01 | 0.01 |
| DiCap | 0.03 | 0.02 | 0.02 | 0.05 | <0.01 | 0.01 | 0.01 |
| Tragic | 0.03 | 0.03 | 0.02 | 0.01 | 0.02 | 0.03 | 0.03 |
| Merka | 0.03 | <0.01 | 0.02 | <0.01 | <0.01 | 0.02 | 0.01 |
| Petro | 0.03 | 0.04 | 0.01 | 0.02 | 7.39 | 0.02 | 0.02 |
| M&M | 0.05 | 0.04 | n/a | 0.02 | 0.05 | 0.04 | 0.04 |
| Signed | 0.03 | 0.03 | 0.05 | 0.03 | 0.51 | 0.06 | 0.06 |
| Light | 0.04 | 0.16 | 0.10 | 0.10 | >600 | 0.03 | 0.03 |
| Forever | 0.14 | 1.54 | 0.20 | 2.31 | 0.80 | 3.05 | 0.03 |
| Center | 0.04 | 3.12 | 2.25 | 0.02 | <0.01 | 0.13 | 0.30 |
| Hot | 0.03 | 0.36 | 0.08 | 0.03 | >600 | 0.05 | 0.05 |
| Karate | 0.03 | 0.39 | 0.05 | 0.02 | 8.57 | 0.03 | 0.07 |
| 9-Dom | 0.43 | 4.74 | 10.69 | 9.90 | 82.14 | 10.79 | 1.93 |
| Flag | 0.05 | 0.18 | n/a | 0.01 | 0.54 | 0.03 | 0.07 |
| Lion | 0.05 | 2.97 | 28.83 | 4.01 | >600 | 0.52 | 3.66 |
| Marley | 0.55 | >600 | 28.02 | 0.73 | >600 | 0.29 | 7.29 |
| Thing | 1.91 | 179 | 285 | 124 | >600 | 276 | 8.73 |
| Nature | 1.05 | >600 | 55.97 | 1.97 | 521 | 71.90 | 25.05 |
| Sierp | 2.36 | >600 | >600 | 5.41 | >600 | >600 | 123 |
| Gettys | 7.31 | >600 | n/a | >600 | err | >600 | 211 |
| Knotty | 1.22 | >600 | >600 | >600 | >600 | >600 | >600 |
| Meow | 172 | >600 | n/a | >600 | >600 | >600 | >600 |
| Faase | 15.94 | >600 | n/a | >600 | >600 | >600 | >600 |

### Hardest webpbn puzzles

| puzzle | nonosolve | pbnsolve | naughty | jsolver | grid | nonogrid_bt | nonogrid_sat |
|---|---:|---:|---:|---:|---:|---:|---:|
| #3867 | 0.43 | 0.04 | n/a | >600 | err | 96.02 | 5.97 |
| #13480 | 0.08 | 124 | 75.19 | 81.37 | >600 | >600 | 6.25 |
| #16900 | 0.76 | 44.58 | n/a | 0.94 | >600 | 72.12 | 9.38 |
| #19080 | 0.50 | 5.98 | 45.82 | 0.07 | >600 | 1.12 | 283 |
| #25385 | 9.06 | >600 | >600 | >600 | >600 | >600 | 253 |
| #25820 | 632 | >600 | n/a | >600 | >600 | >600 | >600 |
| #27174 | 1.01 | 3.56 | 13.56 | 67.45 | >600 | 32.27 | 1.36 |
| #30509 | 1.15 | 0.50 | n/a | 31.73 | 0.08 | 22.99 | 1.21 |
| #30532 | 0.10 | 5.20 | 10.21 | 0.79 | 484 | 1.96 | 9.88 |
| #30654 | 0.45 | >600 | 32.68 | >600 | 1.72 | 2.01 | >600 |
| #30681 | 0.34 | >600 | 74.54 | 0.41 | >600 | 63.05 | 2.81 |
| #32013 | 0.14 | 0.10 | 16.61 | 0.07 | >600 | 1.39 | 10.21 |
| #32291 | 1.74 | 0.03 | 4.34 | 0.47 | 23.51 | 29.64 | 51.90 |

### Totals over all 44 puzzles (600 s limit; unsolved counted as 600 s)

| solver | solved | total CPU s |
|---|---:|---:|
| nonosolve | 43 | 819 |
| pbnsolve | 33 | 6976 |
| naughty | 32 (9 too large) | - |
| jsolver | 36 | 5132 |
| grid | 25 | 12530 |
| nonogrid_bt | 36 | 5486 |
| nonogrid_sat | 39 | 4016 |
| best competitor per puzzle | 40 | 2895 |

### nonosolve with different random seeds

The tables above show one run with the default seed 0.  On the hardest puzzles the run time depends strongly on the seed (`--seed`), which changes the decision order of the SAT solvers.

| puzzle | seed 0 | seed 1 | seed 2 | seed 3 | seed 4 | median |
|---|---:|---:|---:|---:|---:|---:|
| Knotty | 1.22 | 1.15 | 1.15 | 1.15 | 1.16 | 1.15 |
| Meow | 172 | 76.92 | 37.61 | 130 | 165 | 130 |
| Faase | 15.94 | 37.85 | 53.81 | 14.39 | 14.53 | 15.94 |
| #25820 | 632 | 564 | 566 | 533 | 486 | 564 |

## Tournament sets: nonosolve vs LalaFrogKK

| set | nonosolve solved | nonosolve CPU s | LalaFrogKK solved | LalaFrogKK CPU s | speed-up |
|---|---:|---:|---:|---:|---:|
| icga2016 | 1000 | 63.9 | 1000 | 714.6 | 11.2x |
| icga2017 | 1000 | 83.4 | 946 | >7200 (time limit) |  |
| icga2018 | 1000 | 92.5 | 1000 | 1527.3 | 16.5x |
| icga2019 | 1000 | 75.4 | 1000 | 680.2 | 9.0x |
| icga2020 | 1000 | 82.2 | 1000 | 2590.3 | 31.5x |
| icga2021 | 1000 | 82.1 | 1000 | 1260.0 | 15.3x |
| icga2022 | 1000 | 80.7 | 1000 | 1288.4 | 16.0x |
| icga2023 | 1000 | 89.2 | 1000 | 4244.6 | 47.6x |
| taai2011 | 1000 | 71.4 | 1000 | 463.4 | 6.5x |
| taai2013 | 1000 | 79.4 | 1000 | 1546.3 | 19.5x |
| taai2014 | 1000 | 60.7 | 1000 | 584.4 | 9.6x |
| taai2015 | 1000 | 93.0 | 1000 | 1945.3 | 20.9x |
| taai2016 | 1000 | 69.7 | 1000 | 786.5 | 11.3x |
| taai2017 | 1000 | 69.7 | 1000 | 3338.7 | 47.9x |
| taai2018 | 1000 | 61.4 | 1000 | 865.5 | 14.1x |
| taai2019 | 1000 | 76.8 | 1000 | 799.2 | 10.4x |
| taai2020 | 1000 | 77.4 | 1000 | 4104.5 | 53.0x |
| taai2021 | 1000 | 75.2 | 1000 | 1130.8 | 15.0x |
| taai2022 | 1000 | 67.2 | 1000 | 909.3 | 13.5x |
| tcga2012 | 1000 | 85.3 | 1000 | 1130.9 | 13.3x |
| tcga2013 | 1000 | 72.6 | 1000 | 610.2 | 8.4x |
| tcga2014 | 1000 | 81.4 | 1000 | 1459.1 | 17.9x |
| tcga2015 | 1000 | 66.8 | 1000 | 639.7 | 9.6x |
| tcga2016 | 1000 | 81.3 | 970 | >7200 (time limit) |  |
| tcga2017 | 1000 | 72.3 | 1000 | 3437.5 | 47.5x |
| tcga2019 | 1000 | 79.5 | 1000 | 823.2 | 10.4x |
| tcga2020 | 1000 | 76.8 | 1000 | 1025.8 | 13.4x |
| tcga2021 | 1000 | 76.7 | 1000 | 1384.6 | 18.1x |
| tcga2022 | 1000 | 66.7 | 1000 | 562.3 | 8.4x |
| tcga2023 | 1000 | 64.9 | 1000 | 1264.9 | 19.5x |
| **sets both finished** | | **2111** | | **41117** | **19.5x** |

nonosolve per puzzle: median 0.010 s, 99th percentile 0.90 s, slowest 12.2 s. Puzzles over 10 s: nonosolve 1, LalaFrogKK 385.

## 5000 random 30x30 puzzles

| solver | <0.1 | 0.1-0.2 | 0.2-0.5 | 0.5-1 | 1-4 | 4-10 | 10-30 | 30-60 | 60-120 | >120 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| nonosolve (this server) | 4953 | 17 | 13 | 14 | 3 | 0 | 0 | 0 | 0 | 0 |
| Syromolotov/JSolver * | 4417 | 313 | 129 | 40 | 41 | 26 | 13 | 4 | 5 | 12 |
| Wolter/pbnsolve * | 4362 | 221 | 169 | 70 | 88 | 38 | 21 | 6 | 7 | 18 |
| Wu/Naughty * | 4075 | 321 | 229 | 122 | 171 | 57 | 18 | 2 | 1 | 4 |
| BGU * | 0 | 23 | 2852 | 1790 | 284 | 22 | 12 | 8 | 4 | 5 |
| Tamura/Copris * | 0 | 0 | 0 | 0 | 0 | 4691 | 303 | 5 | 0 | 1 |

nonosolve: all 5000 solved, slowest 2.25 s, total 98 s; 124 unique, 4876 with several solutions. * = numbers quoted from Wolter's survey (his hardware), not re-measured.
