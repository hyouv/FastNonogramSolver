# FastNonogramSolver

`nonosolve` solves black-and-white nonograms (paint-by-number puzzles) and can
prove that a solution is unique.

On one server, single-threaded, against the best public solvers:

* **webpbn puzzles** (Jan Wolter's survey set plus the hardest webpbn puzzles,
  44 in all, find up to two solutions):
  * nonosolve solves 43 within 600 s; the best competitor solves 39.
  * Knotty, Meow and Faase, which no solver in Wolter's survey finished, take
    1.2 s, 130 s and 16 s (median of five random seeds; the ranges are
    1.1–1.2 s, 38–172 s and 14–54 s).  None of the six competitors solves
    them within 600 s.
  * webpbn #25820 is proved unique in 564 s (median of five seeds,
    486–632 s).  The nonogrid project reports about 29.6 hours for it.
* **TAAI/TCGA/ICGA tournament sets** (30 × 1000 puzzles):
  * nonosolve solves every set in 61–93 s.
  * LalaFrogKK, the open-source tournament champion, needs 19.5× as much time
    in total on the 28 sets it finishes (6.5–53.0× per set).
  * LalaFrogKK hits the 2-hour limit on the other two sets (icga2017 and
    tcga2016).
* **5000 random 30x30 puzzles**: all solved with a uniqueness check, the
  slowest in 2.3 s.

It is not fastest everywhere.  On some easy and medium puzzles, mostly ones
with several solutions, the best competitor is faster, by up to 60× (#32291:
1.7 s, against 0.03 s for pbnsolve).  None of these takes nonosolve more than
1.8 s.  Full tables and raw numbers are in [results/](results/README.md).

## How it works

1. **A bit-parallel line solver** (`src/linesolve.h`).  For one line it computes
   every cell that is black (or white) in all completions, or reports a conflict.
   The DP runs over block indices, and each step works on whole lines as
   bitmasks:
   * reachability "through non-black cells" uses one addition, because the carry
     sweeps a run of ones: `fill(S, M) = ((M + (S & M)) ^ M) | S`;
   * "block of length c fits at s" and "spread a start set over c cells" take
     O(log c) shifts;
   * the backward pass is the forward pass on the mirrored line.

   That makes O(k log c) word operations per line, with multi-word bitsets for
   lines longer than 62 cells.  It is checked against brute force and a scalar
   DP (`tests/`).
2. **Propagation and full probing** (`src/engine.h`).
   * Dirty lines are solved to a fixpoint.
   * Probing then tries both values of every unknown cell in place, undoing
     through a trail, and keeps what the two branches agree on.
   * A probe is skipped when none of the lines it touched last time has changed
     since.
   * Line results are cached.
3. **DFS with restarts.**
   * Probing results give the branching score.  The score formula is the one
     used by LalaFrogKK (see Sources).
   * Luby restarts alternate the value order (white first, then black first) and
     add noise from the third run on.
4. **CDCL fallback** (`src/sat.h`, `src/sat.cpp`).  If the DFS exceeds a work
   budget, the probed state is encoded to CNF:
   * the encoding uses order variables ("block j starts at or before p") and
     cover variables;
   * binary clauses learned while probing are added, and so are clauses that
     rule out every block start that no completion of its line allows (unit
     propagation on the encoding cannot derive these);
   * **phase hints from belief propagation**: rows and columns exchange
     per-cell probabilities for a few rounds.  Each line computes its message
     exactly, by forward-backward counting of its completions weighted by the
     crossing lines' messages.  The result gives a likely value for every cell
     and every order and cover variable;
   * CaDiCaL finds the first solution, with the hints as forced decision
     phases;
   * a quick 2x2-swap neighbourhood check often gives a second solution at no
     cost;
   * otherwise CaDiCaL, guided by the first solution, looks for nearby
     solutions, and Kissat proves uniqueness if CaDiCaL does not find one within
     its budget.  Kissat gets the hints as initial phases (it has no phase
     API, so variables whose hint is "false" are renamed to their negation).

## Build

```
deps/build_sat.sh        # fetches and builds CaDiCaL 3.0.1 and Kissat 4.0.4 into third_party/
make                     # or: make CXX=g++
```
Kissat and CaDiCaL both contain a copy of "kitten".  `deps/build_sat.sh`
renames Kissat's copy (`deps/kissat_rename_dups.h`) so that both libraries link
into one binary.

Line-solver tests: `c++ -std=c++17 -O2 tests/test_line.cpp -o test_line && ./test_line`.

## Usage

```
bin/nonosolve puzzle.nin                 # solve + uniqueness check (finds up to 2 solutions)
bin/nonosolve -n 1 puzzle.cwd            # any solution
bin/nonosolve --taai set.txt -o sol.txt --log log.txt   # TAAI/TCGA/ICGA tournament batch format
```
Formats: `.nin`, `.cwd`, `.non` (Simpson), and the TAAI batch format.  Useful
options are `--timeout S`, `--sat none|hybrid|kissat|cadical`, `--enc 0|1|2`,
`--dfs-work W`, and `--no-sat-phase` / `--no-sat-starts` to switch off the
phase hints and the block-start clauses.

## Repository layout

```
src/, tests/, Makefile    the solver
deps/                     SAT solver build script
puzzles/fetch.sh          downloads the puzzles that may not be redistributed
puzzles/survey/           Wolter's survey sample set (14 of 31 included, rest via fetch.sh)
puzzles/hard/             hardest webpbn puzzles (13, via fetch.sh)
puzzles/tournament/       TAAI/TCGA/ICGA tournament sets, 2011-2023 (30 x 1000 puzzles)
puzzles/rand30/           Wolter's 5000 random 30x30 puzzles (via fetch.sh)
results/                  benchmark results (CSV) and summary tables
solutions/                solutions of Knotty and Faase
bench/                    benchmark scripts
```

## Reproducing the benchmarks

```
deps/build_sat.sh && make CXX=g++
puzzles/fetch.sh               # downloads the webpbn and random puzzles
bench/build_competitors.sh     # fetches and builds the competitor solvers (Linux; needs cargo for nonogrid)
bench/run_all.sh               # full evaluation on CPUs 0-7, then writes results/
```
* `bench/run_bench.py` runs any of the solvers on puzzle files, with CPU
  pinning and timeouts.
* `bench/verify_taai.py` checks tournament solution files.

## Sources

### Puzzles
Puzzles on webpbn.com belong to their designers and may only be downloaded for
personal use.  This repository therefore includes only puzzles whose authors
allow redistribution.  `puzzles/fetch.sh` downloads the rest from the original
sites; the downloaded files are byte-identical to the ones benchmarked.

* `puzzles/survey`:
  * the sample set of Jan Wolter's
    [Survey of Paint-by-Number Puzzle Solvers](https://webpbn.com/survey/);
  * the 14 included puzzles are freely redistributable with attribution;
    `puzzles/survey/README.md` lists their copyright holders;
  * Knotty is by Joe Cooke and Faase by Kerrin Mansfield.  This also covers
    their solutions in `solutions/`.
* `puzzles/hard`:
  * webpbn puzzles 3867, 13480, 16900, 19080, 25385, 25820, 27174, 30509,
    30532, 30654, 30681, 32013 and 32291;
  * the list of the hardest webpbn puzzles comes from the
    [nonogrid](https://github.com/tsionyx/nonogrid) benchmarks;
  * downloaded from the webpbn [export page](https://webpbn.com/export.cgi).
* `puzzles/tournament`:
  * the TAAI, TCGA and ICGA computer nonogram tournament sets;
  * copied from [zxkyjimmy/NonogramRecord](https://github.com/zxkyjimmy/NonogramRecord)
    and licensed under GPL-3.0 (`puzzles/tournament/LICENSE`).
* `puzzles/rand30`: Wolter's 5000 random 30x30 puzzles, downloaded from
  [rand30.tgz](https://webpbn.com/survey/rand30.tgz).

### Competitor solvers
* **LalaFrogKK**:
  * by Kan-Yueh Chen, Ching-Hua Kuo, Hao-Hua Kang, Der-Johng Sun and I-Chen Wu
    (CGI Lab, NCTU);
  * this version won every TAAI, TCGA and Computer Olympiad nonogram tournament
    from TAAI 2011 to Computer Olympiad 2015;
  * source: [CGI-LAB/Nonogram](https://github.com/CGI-LAB/Nonogram), commit `3f15247`;
  * built single-threaded;
  * algorithm: I-Chen Wu, Der-Johng Sun, Lung-Ping Chen, Kan-Yueh Chen,
    Ching-Hua Kuo, Hao-Hua Kang and Hung-Hsuan Lin, *An Efficient Approach to
    Solving Nonograms*, IEEE Transactions on Computational Intelligence and AI
    in Games 5(3):251–264, 2013.
* **nonogrid** by tsionyx:
  * source: [tsionyx/nonogrid](https://github.com/tsionyx/nonogrid) v0.7.3;
  * built twice, as the backtracking solver (`nonogrid_bt`) and with its SAT
    backend (`nonogrid_sat`).
* **pbnsolve** 1.09 by Jan Wolter:
  [archive](https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/pbnsolve/pbnsolve-1.09.tgz).
* **Naughty** v88 by Kuang-che Wu:
  [source](http://kcwu.csie.org/~kcwu/nonogram/naughty/).
* **JSolver** 1.4 by Evgeniy Syromolotov:
  [SourceForge](https://sourceforge.net/projects/jsolver/).
* **grid** by Mirek and Petr Olšák:
  [source](http://petr.olsak.net/ftp/olsak/grid/).

Numbers marked as quoted in `results/` come from Wolter's survey and from the
nonogrid benchmark table (`benches/perf.csv` in the nonogrid repository).

## References

nonosolve links two SAT solvers by Armin Biere and colleagues:

* **CaDiCaL** ([github.com/arminbiere/cadical](https://github.com/arminbiere/cadical), MIT License):
  Armin Biere, Tobias Faller, Katalin Fazekas, Mathias Fleury, Nils Froleyks and Florian Pollitt.
  *CaDiCaL 2.0.*  In Proc. Computer Aided Verification (CAV 2024), LNCS vol. 14681, pp. 133–152, Springer, 2024.
* **Kissat** ([github.com/arminbiere/kissat](https://github.com/arminbiere/kissat), MIT License):
  Armin Biere, Tobias Faller, Katalin Fazekas, Mathias Fleury, Nils Froleyks and Florian Pollitt.
  *CaDiCaL, Gimsatul, IsaSAT and Kissat Entering the SAT Competition 2024.*
  In Proc. SAT Competition 2024: Solver, Benchmark and Proof Checker Descriptions,
  Department of Computer Science Report Series B, vol. B-2024-1, pp. 8–10, University of Helsinki, 2024.

## License

The code is released under the [MIT License](LICENSE).  The puzzle files keep
their own terms:
* `puzzles/tournament` is under GPL-3.0;
* the puzzles in `puzzles/survey` may be redistributed only with the
  attribution listed in `puzzles/survey/README.md`.
