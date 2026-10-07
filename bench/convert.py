#!/usr/bin/env python3
"""Convert .nin puzzles into the input formats of the competitor solvers."""
import os, sys

def read_nin(path):
    lines = [l for l in open(path).read().split('\n')]
    w, h = map(int, lines[0].split())
    rest = [l for l in lines[1:] if l.strip() != '']
    clues = [[int(x) for x in l.split() if int(x) > 0] for l in rest[:h + w]]
    return w, h, clues[:h], clues[h:]

def write_all(src, outdir):
    w, h, rows, cols = read_nin(src)
    base = os.path.splitext(os.path.basename(src))[0]
    os.makedirs(outdir, exist_ok=True)
    # Simpson .non (Naughty)
    with open(os.path.join(outdir, base + '.non'), 'w') as f:
        f.write('width %d\nheight %d\n\nrows\n' % (w, h))
        for r in rows: f.write((','.join(map(str, r)) or '0') + '\n')
        f.write('\ncolumns\n')
        for c in cols: f.write((','.join(map(str, c)) or '0') + '\n')
    # Syromolotov (JSolver)
    with open(os.path.join(outdir, base + '.syro'), 'w') as f:
        for r in rows: f.write(' '.join(map(str, r + [0])) + '\n')
        f.write('#\n')
        for c in cols: f.write(' '.join(map(str, c + [0])) + '\n')
        f.write('#\n')
    # Olsak .g (grid)
    with open(os.path.join(outdir, base + '.g'), 'w') as f:
        f.write('#d\n   0:   #FFFFFF   white\n   g:X  #000000   black\n: rows\n')
        for r in rows: f.write(' '.join('%dg' % x for x in r) + '\n')
        f.write(': columns\n')
        for c in cols: f.write(' '.join('%dg' % x for x in c) + '\n')

if __name__ == '__main__':
    outdir = sys.argv[1]
    for p in sys.argv[2:]:
        write_all(p, outdir)
