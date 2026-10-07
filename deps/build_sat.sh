#!/bin/bash
# Fetch and build the SAT solvers nonosolve links against (CaDiCaL 3.0.1, Kissat 4.0.4).
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
TP=$ROOT/third_party
J=${J:-8}
mkdir -p $TP
fetch() {  # fetch <dir> <url> <commit>
  [ -d $TP/$1 ] || git clone -q $2 $TP/$1
  git -C $TP/$1 checkout -q $3
}
fetch cadical https://github.com/arminbiere/cadical.git c60730422e758ef1cebe7aeddf2dda31c996bf04
fetch kissat https://github.com/arminbiere/kissat.git 8af8e56f174b778aef3aa45af9f739b2a5f492c2

(cd $TP/cadical && rm -f src/makefile && ./configure && make -j$J)
# Kissat and CaDiCaL both contain "kitten"; rename Kissat's copy so both link into one binary.
(cd $TP/kissat && rm -f src/makefile && ./configure --compact &&
 sed -i.bak "1s|\$| -include $ROOT/deps/kissat_rename_dups.h|" build/makefile && make -j$J)
