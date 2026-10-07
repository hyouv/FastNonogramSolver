#!/usr/bin/env python3
"""Verify a TAAI-format solution file against the puzzle file."""
import re, sys

def blocks(text):
    out = {}
    cur = None
    for line in text.replace('\r', '').split('\n'):
        if line.startswith('$'):
            cur = line[1:].strip(); out[cur] = []
        elif cur is not None:
            out[cur].append(line)
    return out

def runs(v):
    return [len(m) for m in re.findall('1+', ''.join(map(str, v)))]

def main(puz, sol):
    P = blocks(open(puz).read()); S = blocks(open(sol).read())
    bad = []
    for k, lines in P.items():
        clues = [[int(x) for x in l.split() if int(x) > 0] for l in lines]
        while len(clues) % 2: clues.pop()
        n = len(clues) // 2
        cols, rows = clues[:n], clues[n:]
        g = [[int(x) for x in l.split()] for l in S.get(k, []) if l.strip()]
        ok = len(g) == n and all(len(r) == n for r in g)
        if ok:
            ok = all(runs(g[i]) == rows[i] for i in range(n)) and \
                 all(runs([g[i][j] for i in range(n)]) == cols[j] for j in range(n))
        if not ok: bad.append(k)
    print(f"{len(P) - len(bad)}/{len(P)} correct" + (f"; wrong: {bad[:20]}" if bad else ""))
    return 1 if bad else 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1], sys.argv[2]))
