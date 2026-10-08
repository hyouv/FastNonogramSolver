# Benchmark results

All numbers were measured on one server (2x Intel Xeon Gold 5416S, Ubuntu 20.04, gcc 9.4), one process per physical core pinned with `taskset`, at most 8 jobs at a time.  Times are CPU seconds (user + system).

* **webpbn puzzles**: find up to two solutions, i.e. solve and check uniqueness.  Competitors had 600 s; nonosolve had 3600 s, and only #25820 (721 s) needed more than 600 s.  `n/a` = puzzle too large for the solver, `err` = the solver rejected the input.
* **Tournament sets** (TAAI/TCGA/ICGA, 1000 puzzles of 25x25 each): first solution of every puzzle, in order, with the tournament limit of 2 hours per set.
* **Random 30x30 puzzles** (Wolter): solve and check uniqueness, 120 s limit.

Files: `webpbn.csv`, `tournament_sets.csv`, `tournament_puzzles.csv` (per-puzzle times), `rand30.csv`.

## webpbn puzzles

### Wolter survey sample set

| puzzle | nonosolve | pbnsolve | naughty | jsolver | grid | nonogrid_bt | nonogrid_sat |
|---|---:|---:|---:|---:|---:|---:|---:|
| Dancer | 0.02 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
| Cat | 0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
| Skid | 0.02 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
| Bucks | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
| Edge | 0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
| Smoke | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 | <0.01 |
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
| Forever | 0.45 | 1.54 | 0.20 | 2.31 | 0.80 | 3.05 | 0.03 |
| Center | 0.04 | 3.12 | 2.25 | 0.02 | <0.01 | 0.13 | 0.30 |
| Hot | 0.04 | 0.36 | 0.08 | 0.03 | >600 | 0.05 | 0.05 |
| Karate | 0.03 | 0.39 | 0.05 | 0.02 | 8.57 | 0.03 | 0.07 |
| 9-Dom | 1.17 | 4.74 | 10.69 | 9.90 | 82.14 | 10.79 | 1.93 |
| Flag | 0.05 | 0.18 | n/a | 0.01 | 0.54 | 0.03 | 0.07 |
| Lion | 0.06 | 2.97 | 28.83 | 4.01 | >600 | 0.52 | 3.66 |
| Marley | 0.98 | >600 | 28.02 | 0.73 | >600 | 0.29 | 7.29 |
| Thing | 4.52 | 179 | 285 | 124 | >600 | 276 | 8.73 |
| Nature | 1.30 | >600 | 55.97 | 1.97 | 521 | 71.90 | 25.05 |
| Sierp | 0.77 | >600 | >600 | 5.41 | >600 | >600 | 123 |
| Gettys | 10.80 | >600 | n/a | >600 | err | >600 | 211 |
| Knotty | 7.47 | >600 | >600 | >600 | >600 | >600 | >600 |
| Meow | 137 | >600 | n/a | >600 | >600 | >600 | >600 |
| Faase | 10.11 | >600 | n/a | >600 | >600 | >600 | >600 |

### Hardest webpbn puzzles

| puzzle | nonosolve | pbnsolve | naughty | jsolver | grid | nonogrid_bt | nonogrid_sat |
|---|---:|---:|---:|---:|---:|---:|---:|
| #3867 | 0.81 | 0.04 | n/a | >600 | err | 96.02 | 5.97 |
| #13480 | 0.08 | 124 | 75.19 | 81.37 | >600 | >600 | 6.25 |
| #16900 | 1.55 | 44.58 | n/a | 0.94 | >600 | 72.12 | 9.38 |
| #19080 | 0.13 | 5.98 | 45.82 | 0.07 | >600 | 1.12 | 283 |
| #25385 | 3.54 | >600 | >600 | >600 | >600 | >600 | 253 |
| #25820 | 721 | >600 | n/a | >600 | >600 | >600 | >600 |
| #27174 | 1.54 | 3.56 | 13.56 | 67.45 | >600 | 32.27 | 1.36 |
| #30509 | 2.02 | 0.50 | n/a | 31.73 | 0.08 | 22.99 | 1.21 |
| #30532 | 0.10 | 5.20 | 10.21 | 0.79 | 484 | 1.96 | 9.88 |
| #30654 | 0.61 | >600 | 32.68 | >600 | 1.72 | 2.01 | >600 |
| #30681 | 0.62 | >600 | 74.54 | 0.41 | >600 | 63.05 | 2.81 |
| #32013 | 0.14 | 0.10 | 16.61 | 0.07 | >600 | 1.39 | 10.21 |
| #32291 | 0.19 | 0.03 | 4.34 | 0.47 | 23.51 | 29.64 | 51.90 |

### Totals over all 44 puzzles (600 s limit; unsolved counted as 600 s)

| solver | solved | total CPU s |
|---|---:|---:|
| nonosolve | 43 | 786 |
| pbnsolve | 33 | 6976 |
| naughty | 32 (9 too large) | - |
| jsolver | 36 | 5132 |
| grid | 25 | 12530 |
| nonogrid_bt | 36 | 5486 |
| nonogrid_sat | 39 | 4016 |
| best competitor per puzzle | 40 | 2895 |

## Tournament sets: nonosolve vs LalaFrogKK

| set | nonosolve solved | nonosolve CPU s | LalaFrogKK solved | LalaFrogKK CPU s | speed-up |
|---|---:|---:|---:|---:|---:|
| icga2016 | 1000 | 76.7 | 1000 | 714.6 | 9.3x |
| icga2017 | 1000 | 93.6 | 946 | >7200 (time limit) |  |
| icga2018 | 1000 | 104.1 | 1000 | 1527.3 | 14.7x |
| icga2019 | 1000 | 83.0 | 1000 | 680.2 | 8.2x |
| icga2020 | 1000 | 92.6 | 1000 | 2590.3 | 28.0x |
| icga2021 | 1000 | 96.2 | 1000 | 1260.0 | 13.1x |
| icga2022 | 1000 | 91.1 | 1000 | 1288.4 | 14.1x |
| icga2023 | 1000 | 104.0 | 1000 | 4244.6 | 40.8x |
| taai2011 | 1000 | 83.7 | 1000 | 463.4 | 5.5x |
| taai2013 | 1000 | 87.9 | 1000 | 1546.3 | 17.6x |
| taai2014 | 1000 | 64.0 | 1000 | 584.4 | 9.1x |
| taai2015 | 1000 | 108.9 | 1000 | 1945.3 | 17.9x |
| taai2016 | 1000 | 77.8 | 1000 | 786.5 | 10.1x |
| taai2017 | 1000 | 76.8 | 1000 | 3338.7 | 43.4x |
| taai2018 | 1000 | 72.4 | 1000 | 865.5 | 12.0x |
| taai2019 | 1000 | 94.1 | 1000 | 799.2 | 8.5x |
| taai2020 | 1000 | 90.7 | 1000 | 4104.5 | 45.2x |
| taai2021 | 1000 | 91.2 | 1000 | 1130.8 | 12.4x |
| taai2022 | 1000 | 78.5 | 1000 | 909.3 | 11.6x |
| tcga2012 | 1000 | 106.8 | 1000 | 1130.9 | 10.6x |
| tcga2013 | 1000 | 92.5 | 1000 | 610.2 | 6.6x |
| tcga2014 | 1000 | 93.2 | 1000 | 1459.1 | 15.7x |
| tcga2015 | 1000 | 70.4 | 1000 | 639.7 | 9.1x |
| tcga2016 | 1000 | 88.0 | 970 | >7200 (time limit) |  |
| tcga2017 | 1000 | 85.0 | 1000 | 3437.5 | 40.5x |
| tcga2019 | 1000 | 91.8 | 1000 | 823.2 | 9.0x |
| tcga2020 | 1000 | 77.9 | 1000 | 1025.8 | 13.2x |
| tcga2021 | 1000 | 88.5 | 1000 | 1384.6 | 15.6x |
| tcga2022 | 1000 | 80.2 | 1000 | 562.3 | 7.0x |
| tcga2023 | 1000 | 79.3 | 1000 | 1264.9 | 16.0x |
| **sets both finished** | | **2439** | | **41117** | **16.9x** |

nonosolve per puzzle: median 0.010 s, 99th percentile 1.32 s, slowest 13.1 s. Puzzles over 10 s: nonosolve 2, LalaFrogKK 385.

## 5000 random 30x30 puzzles

| solver | <0.1 | 0.1-0.2 | 0.2-0.5 | 0.5-1 | 1-4 | 4-10 | 10-30 | 30-60 | 60-120 | >120 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| nonosolve (this server) | 4954 | 17 | 9 | 8 | 12 | 0 | 0 | 0 | 0 | 0 |
| Syromolotov/JSolver * | 4417 | 313 | 129 | 40 | 41 | 26 | 13 | 4 | 5 | 12 |
| Wolter/pbnsolve * | 4362 | 221 | 169 | 70 | 88 | 38 | 21 | 6 | 7 | 18 |
| Wu/Naughty * | 4075 | 321 | 229 | 122 | 171 | 57 | 18 | 2 | 1 | 4 |
| BGU * | 0 | 23 | 2852 | 1790 | 284 | 22 | 12 | 8 | 4 | 5 |
| Tamura/Copris * | 0 | 0 | 0 | 0 | 0 | 4691 | 303 | 5 | 0 | 1 |

nonosolve: all 5000 solved, slowest 2.78 s, total 105 s; 124 unique, 4876 with several solutions. * = numbers quoted from Wolter's survey (his hardware), not re-measured.
