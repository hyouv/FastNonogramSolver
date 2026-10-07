#!/bin/bash
# Fetch and build the competitor solvers used in the benchmarks (Linux, gcc).
#   LalaFrogKK (TAAI/TCGA/ICGA champion), nonogrid (backtracking and SAT builds),
#   and the webpbn-survey solvers pbnsolve, Naughty, JSolver and Olsak's grid.
set -x
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TP=$ROOT/third_party
SS=$TP/survey_solvers
mkdir -p $TP $SS
fetch() {  # fetch <dir> <url> <commit>
  [ -d $TP/$1 ] || git clone -q $2 $TP/$1
  git -C $TP/$1 checkout -q $3
}
untar() {  # untar <name> <url>
  [ -d $SS/$1 ] || (mkdir -p $SS/$1 && curl -sfL "$2" | tar -xz -C $SS/$1)
}
fetch CGI-LAB_Nonogram https://github.com/CGI-LAB/Nonogram.git 3f15247fa8405a95891d7785b27c85e3f9b801a3
fetch tsionyx_nonogrid https://github.com/tsionyx/nonogrid.git aaf7f9a4f70352dbc65fb21f6335995d796f8476
untar naughty http://kcwu.csie.org/~kcwu/nonogram/naughty/naughty-v88.tgz
untar jsolver https://sourceforge.net/projects/jsolver/files/jsolver/jsolver-1.4/jsolver-1.4-src.tar.gz/download
untar pbnsolve https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/pbnsolve/pbnsolve-1.09.tgz
untar grid http://petr.olsak.net/ftp/olsak/grid/grid.tgz

# LalaFrogKK, single-threaded (file names in #include differ in case from the files)
(cd $TP/CGI-LAB_Nonogram &&
 ln -sf Hash.h hash.h; ln -sf Puzzle.h puzzle.h; ln -sf LineSolver.h lineSolver.h; ln -sf SearchSolver.h searchSolver.h
 g++ -O3 -march=native -DCLK_TCK=CLOCKS_PER_SEC -include cmath -include cstring -w -o lalafrog $(ls *.cpp | grep -v Scheduling))

# Naughty for boards up to 31 and up to 63 cells
(cd $SS/naughty/naughty-v88 && touch .depend &&
 sed -i 's|^#define LARGE_BOARD|//#define LARGE_BOARD|' config.h && make clean && make naughty CXX=g++ CXXFLAGS="-O3 -DNDEBUG -Wno-narrowing" && cp naughty naughty31 &&
 sed -i 's|^//#define LARGE_BOARD|#define LARGE_BOARD|' config.h && make clean && make naughty CXX=g++ CXXFLAGS="-O3 -DNDEBUG -Wno-narrowing" && cp naughty naughty63)

# pbnsolve without libxml2 (XML input is not needed)
(cd $SS/pbnsolve/pbnsolve-1.09 &&
 sed -i 's|^/\* #define NOXML /\*\*/|#define NOXML /**/|' config.h &&
 make clean; make pbnsolve CC=gcc CFLAGS="-O2 -w -DNOXML" LIB="-lm")

(cd $SS/jsolver/jsolver-1.4-src && gcc -O2 -w -o jsolver jsolver.c)
(cd $SS/grid/grid && gcc -O3 -w -fno-builtin -Dlogf=grid_logf -o grid grid.c -lm)

# nonogrid (Rust), with and without its SAT backend
(cd $TP/tsionyx_nonogrid &&
 cargo build --release --no-default-features --features="args std_time logger sat" && cp target/release/nonogrid nonogrid_sat &&
 cargo build --release --no-default-features --features="args std_time logger" && cp target/release/nonogrid nonogrid_bt)

# puzzle files in the formats the survey solvers read
python3 $ROOT/bench/convert.py $ROOT/puzzles/fmt $ROOT/puzzles/survey/*.nin $ROOT/puzzles/hard/*.nin
