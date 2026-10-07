#!/bin/bash
# Download the puzzles that may not be redistributed (see README.md in each folder).
set -e
P=$(cd "$(dirname "$0")" && pwd)
TMP=$(mktemp -d)
trap 'rm -rf $TMP' EXIT

# Wolter's survey sample set (all 31 puzzles; the redistributable ones are already here, identical)
curl -sfL https://webpbn.com/survey/puzzles/sample-nin.tgz | tar -xz -C $TMP
cp $TMP/sample-nin/*.nin $P/survey/

# hardest webpbn puzzles, from the webpbn export page
for id in 3867 13480 16900 19080 25385 25820 27174 30509 30532 30654 30681 32013 32291; do
  curl -sf https://webpbn.com/export.cgi --data "id=$id&fmt=nin&go=1" -o $P/hard/webpbn-$(printf %05d $id).nin
  sleep 1
done

# Wolter's 5000 random 30x30 grids, turned into clue files
curl -sfL https://webpbn.com/survey/rand30.tgz | tar -xz -C $TMP
python3 - $TMP/30x30-2 $P/rand30 <<'PY'
import os, re, sys
src, dst = sys.argv[1], sys.argv[2]
runs = lambda s: [len(m) for m in re.findall('1+', s)] or [0]
for f in os.listdir(src):
    g = [l.strip() for l in open(os.path.join(src, f)) if l.strip()]
    h, w = len(g), len(g[0])
    rows = [runs(r) for r in g]
    cols = [runs(''.join(g[r][c] for r in range(h))) for c in range(w)]
    with open(os.path.join(dst, 'rand%04d.nin' % int(f[4:])), 'w') as out:
        out.write('%d %d\n' % (w, h) + '\n'.join(' '.join(map(str, x)) for x in rows + cols) + '\n')
PY
echo "survey: $(ls $P/survey/*.nin | wc -l)  hard: $(ls $P/hard/*.nin | wc -l)  rand30: $(ls $P/rand30/*.nin | wc -l)"
