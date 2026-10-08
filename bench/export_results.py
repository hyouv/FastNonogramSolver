#!/usr/bin/env python3
"""Turn the raw benchmark output (bench/results) into the CSV files and summary in results/.

usage: export_results.py [raw-dir] [out-dir] [ours-dir]

ours-dir (default raw-dir) holds nonosolve's survey_ours.csv, rand30_ours.csv,
optionally hard_seeds.csv, and tournament/ (or raw-dir/ours_final/) with one log per set.
"""
import csv, glob, math, os, sys

RAW = sys.argv[1] if len(sys.argv) > 1 else 'bench/results'
OUT = sys.argv[2] if len(sys.argv) > 2 else 'results'
OURS = sys.argv[3] if len(sys.argv) > 3 else RAW
TOUR = os.path.join(OURS, 'tournament')
if not os.path.isdir(TOUR):
    TOUR = os.path.join(RAW, 'ours_final')

SURVEY = [  # (file stem, name) in the order of Wolter's survey table
    ('webpbn-00001', 'Dancer'), ('webpbn-00006', 'Cat'), ('webpbn-00021', 'Skid'), ('webpbn-00027', 'Bucks'),
    ('webpbn-00023', 'Edge'), ('webpbn-02413', 'Smoke'), ('webpbn-00016', 'Knot'), ('webpbn-00529', 'Swing'),
    ('webpbn-00065', 'Mum'), ('webpbn-07604', 'DiCap'), ('webpbn-01694', 'Tragic'), ('webpbn-01611', 'Merka'),
    ('webpbn-00436', 'Petro'), ('webpbn-04645', 'M&M'), ('webpbn-03541', 'Signed'), ('webpbn-00803', 'Light'),
    ('webpbn-06574', 'Forever'), ('webpbn-10810', 'Center'), ('webpbn-02040', 'Hot'), ('webpbn-06739', 'Karate'),
    ('webpbn-08098', '9-Dom'), ('webpbn-02556', 'Flag'), ('webpbn-02712', 'Lion'), ('webpbn-10088', 'Marley'),
    ('webpbn-18297', 'Thing'), ('webpbn-09892', 'Nature'), ('webpbn-12548', 'Sierp'), ('webpbn-22336', 'Gettys'),
    ('knotty', 'Knotty'), ('meow', 'Meow'), ('faase', 'Faase')]
HARD = [('webpbn-%05d' % i, '#%d' % i) for i in
        (3867, 13480, 16900, 19080, 25385, 25820, 27174, 30509, 30532, 30654, 30681, 32013, 32291)]
COMP = ['pbnsolve', 'naughty', 'jsolver', 'grid', 'nonogrid_bt', 'nonogrid_sat']
LIMIT = {'ours': 3600}  # everything else: 600 s
# Wolter's published distribution for the 5000 random puzzles (his hardware)
RAND_PUBLISHED = [('Syromolotov/JSolver', [4417, 313, 129, 40, 41, 26, 13, 4, 5, 12]),
                  ('Wolter/pbnsolve', [4362, 221, 169, 70, 88, 38, 21, 6, 7, 18]),
                  ('Wu/Naughty', [4075, 321, 229, 122, 171, 57, 18, 2, 1, 4]),
                  ('BGU', [0, 23, 2852, 1790, 284, 22, 12, 8, 4, 5]),
                  ('Tamura/Copris', [0, 0, 0, 0, 0, 4691, 303, 5, 0, 1])]
RAND_BINS = [0.1, 0.2, 0.5, 1, 4, 10, 30, 60, 120]
RAND_LABELS = ['<0.1', '0.1-0.2', '0.2-0.5', '0.5-1', '1-4', '4-10', '10-30', '30-60', '60-120', '>120']


def stem(p):
    s = os.path.basename(p).replace('.nin', '')
    return 'webpbn-%05d' % int(s[7:]) if s.startswith('webpbn-') else s


def outcome(r, limit):
    if r['status'] == 'N/A':
        return 'unsupported'
    note = r.get('note', '')
    if 'rror' in note or 'Segmentation' in note or r['status'] in ('ERROR', 'NONE'):
        return 'error'
    if r['status'] == 'TIMEOUT' or not r['cpu'] or float(r['cpu']) >= 0.97 * limit or 'imeout' in note:
        return 'timeout'
    return 'solved'


def fmt(t):
    if t < 0.01:
        return '<0.01'
    return '%.2f' % t if t < 100 else '%.0f' % t


def webpbn(md):
    rows = []
    for path in (os.path.join(OURS, 'survey_ours.csv'), os.path.join(RAW, 'survey_competitors.csv')):
        for r in csv.DictReader(open(path)):
            lim = LIMIT.get(r['solver'], 600)
            o = outcome(r, lim)
            st = r['status'] if r['solver'] == 'ours' and o == 'solved' else ''
            rows.append({'solver': 'nonosolve' if r['solver'] == 'ours' else r['solver'], 'puzzle': stem(r['puzzle']),
                         'limit_s': lim, 'cpu_s': r['cpu'], 'wall_s': r['wall'], 'result': o, 'uniqueness': st})
    order = {s: i for i, (s, _) in enumerate(SURVEY + HARD)}
    sv = ['nonosolve'] + COMP
    rows.sort(key=lambda r: (order[r['puzzle']], sv.index(r['solver'])))
    with open(os.path.join(OUT, 'webpbn.csv'), 'w', newline='') as f:
        w = csv.DictWriter(f, list(rows[0]))
        w.writeheader()
        w.writerows(rows)
    d = {(r['solver'], r['puzzle']): r for r in rows}

    def cell(r):
        if r is None:
            return '-'
        if r['result'] == 'unsupported':
            return 'n/a'
        if r['result'] == 'error':
            return 'err'
        if r['result'] == 'timeout':
            return '>600'
        return fmt(float(r['cpu_s']))

    for title, group in (('Wolter survey sample set', SURVEY), ('Hardest webpbn puzzles', HARD)):
        md.append('\n### %s\n' % title)
        md.append('| puzzle | nonosolve | ' + ' | '.join(COMP) + ' |')
        md.append('|---|' + '---:|' * (len(COMP) + 1))
        for s, name in group:
            cells = [cell(d.get((c, s))) for c in sv]
            md.append('| %s | %s |' % (name, ' | '.join(cells)))
    # totals: unsolved counted as 600 s; nonosolve also judged with the 600 s limit here
    md.append('\n### Totals over all %d puzzles (600 s limit; unsolved counted as 600 s)\n' % len(SURVEY + HARD))
    md.append('| solver | solved | total CPU s |')
    md.append('|---|---:|---:|')
    tot = {}
    for c in sv + ['best competitor per puzzle']:
        n, t, na = 0, 0.0, 0
        for s, _ in SURVEY + HARD:
            if c == 'best competitor per puzzle':
                ts = [float(d[(x, s)]['cpu_s']) for x in COMP if d[(x, s)]['result'] == 'solved']
                v = min(ts) if ts else None
            else:
                r = d[(c, s)]
                if r['result'] == 'unsupported':
                    na += 1
                v = float(r['cpu_s']) if r['result'] == 'solved' else None
            if v is not None and v < 0.97 * 600:
                n += 1
                t += v
            else:
                t += 600
        tot[c] = (n, t, na)
        md.append('| %s | %d%s | %s |' % (c, n, ' (%d too large)' % na if na else '',
                                          '%.0f' % t if not na else '-'))
    seeds(md)
    return d


def seeds(md):
    """nonosolve's spread over random seeds on the hardest puzzles (ours-dir/hard_seeds.csv)."""
    path = os.path.join(OURS, 'hard_seeds.csv')
    if not os.path.exists(path):
        return
    t = {}
    for r in csv.DictReader(open(path)):
        t.setdefault(stem(r['puzzle']), {})[int(r['solver'].split('--seed')[1].split()[0])] = r
    for r in csv.DictReader(open(os.path.join(OURS, 'survey_ours.csv'))):
        if stem(r['puzzle']) in t:
            t[stem(r['puzzle'])][0] = r
    group = [(s, name) for s, name in SURVEY + HARD if s in t]
    ns = sorted({k for s, _ in group for k in t[s]})
    rows = [{'puzzle': s, 'seed': k, 'cpu_s': t[s][k]['cpu'], 'uniqueness': t[s][k]['status']}
            for s, _ in group for k in ns]
    with open(os.path.join(OUT, 'webpbn_seeds.csv'), 'w', newline='') as f:
        w = csv.DictWriter(f, list(rows[0]))
        w.writeheader()
        w.writerows(rows)
    md.append('\n### nonosolve with different random seeds\n')
    md.append('The tables above show one run with the default seed 0.  On the hardest puzzles the run time depends '
              'strongly on the seed (`--seed`), which changes the decision order of the SAT solvers.\n')
    md.append('| puzzle | ' + ' | '.join('seed %d' % k for k in ns) + ' | median |')
    md.append('|---|' + '---:|' * (len(ns) + 1))
    for s, name in group:
        ts = [float(t[s][k]['cpu']) for k in ns]
        md.append('| %s | %s | %s |' % (name, ' | '.join(fmt(x) for x in ts), fmt(sorted(ts)[len(ts) // 2])))


def per_puzzle(path):
    out = {}
    if os.path.exists(path):
        for line in open(path):
            if line.startswith('#'):
                p = line.split('\t')
                out[int(p[0][1:])] = float(p[1])
    return out


def cpu_of(path):
    if not os.path.exists(path) or not open(path).read().split():
        return None
    u, s, e = open(path).read().split()[-3:]
    return float(u) + float(s), float(e)


def tournament(md):
    sets = sorted(os.path.basename(f)[:-4] for f in glob.glob('puzzles/tournament/*.txt'))
    srows, prow = [], []
    for name in sets:
        O = per_puzzle(os.path.join(TOUR, name + '.log'))
        L = per_puzzle(os.path.join(RAW, 'lala', name, 'log.txt'))
        oc, lc = cpu_of(os.path.join(TOUR, name + '.time')), cpu_of(os.path.join(RAW, 'lala', name, 'time.txt'))
        lfin = 'total time' in open(os.path.join(RAW, 'lala', name, 'log.txt')).read()
        if lc is None:
            print('LalaFrogKK still running on', name)
            continue
        srows.append({'set': name, 'nonosolve_solved': len(O), 'nonosolve_cpu_s': '%.2f' % oc[0],
                      'lalafrogkk_solved': len(L), 'lalafrogkk_cpu_s': '%.2f' % lc[0],
                      'lalafrogkk_hit_2h_limit': 'no' if lfin else 'yes'})
        for i in range(1, 1001):
            prow.append({'set': name, 'puzzle': i, 'nonosolve_s': '%.6f' % O[i] if i in O else '',
                         'lalafrogkk_s': '%.6f' % L[i] if i in L else ''})
    for fn, rows in (('tournament_sets.csv', srows), ('tournament_puzzles.csv', prow)):
        with open(os.path.join(OUT, fn), 'w', newline='') as f:
            w = csv.DictWriter(f, list(rows[0]))
            w.writeheader()
            w.writerows(rows)
    md.append('\n| set | nonosolve solved | nonosolve CPU s | LalaFrogKK solved | LalaFrogKK CPU s | speed-up |')
    md.append('|---|---:|---:|---:|---:|---:|')
    to = tl = 0
    for r in srows:
        o, l = float(r['nonosolve_cpu_s']), float(r['lalafrogkk_cpu_s'])
        lim = r['lalafrogkk_hit_2h_limit'] == 'yes'
        md.append('| %s | %d | %.1f | %d | %s | %s |' % (r['set'], r['nonosolve_solved'], o, r['lalafrogkk_solved'],
                                                       '>7200 (time limit)' if lim else '%.1f' % l,
                                                       '' if lim else '%.1fx' % (l / o)))
        if not lim:
            to += o
            tl += l
    md.append('| **sets both finished** | | **%.0f** | | **%.0f** | **%.1fx** |' % (to, tl, tl / to))
    # per-puzzle view
    n = lf = big_o = big_l = 0
    for name in sets:
        O = per_puzzle(os.path.join(TOUR, name + '.log'))
        L = per_puzzle(os.path.join(RAW, 'lala', name, 'log.txt'))
        for i in O:
            big_o += O[i] > 10
        for i in L:
            big_l += L[i] > 10
    allo = sorted(t for name in sets for t in per_puzzle(os.path.join(TOUR, name + '.log')).values())
    md.append('\nnonosolve per puzzle: median %.3f s, 99th percentile %.2f s, slowest %.1f s. '
              'Puzzles over 10 s: nonosolve %d, LalaFrogKK %d.' % (allo[len(allo) // 2], allo[int(0.99 * len(allo))],
                                                                  allo[-1], big_o, big_l))


def rand30(md):
    rows = []
    for r in csv.DictReader(open(os.path.join(OURS, 'rand30_ours.csv'))):
        rows.append({'puzzle': stem(r['puzzle']), 'cpu_s': r['cpu'], 'wall_s': r['wall'],
                     'result': outcome(r, 120), 'uniqueness': r['status']})
    rows.sort(key=lambda r: r['puzzle'])
    with open(os.path.join(OUT, 'rand30.csv'), 'w', newline='') as f:
        w = csv.DictWriter(f, list(rows[0]))
        w.writeheader()
        w.writerows(rows)
    cnt = [0] * 10
    for r in rows:
        t = float(r['cpu_s']) if r['result'] == 'solved' else 1e9
        i = 0
        while i < 9 and t >= RAND_BINS[i]:
            i += 1
        cnt[i] += 1
    md.append('\n| solver | ' + ' | '.join(RAND_LABELS) + ' |')
    md.append('|---|' + '---:|' * 10)
    md.append('| nonosolve (this server) | ' + ' | '.join(map(str, cnt)) + ' |')
    for name, c in RAND_PUBLISHED:
        md.append('| %s * | %s |' % (name, ' | '.join(map(str, c))))
    ts = sorted(float(r['cpu_s']) for r in rows)
    st = {}
    for r in rows:
        st[r['uniqueness']] = st.get(r['uniqueness'], 0) + 1
    md.append('\nnonosolve: all %d solved, slowest %.2f s, total %.0f s; %d unique, %d with several solutions. '
              '* = numbers quoted from Wolter\'s survey (his hardware), not re-measured.'
              % (len(rows), ts[-1], sum(ts), st.get('UNIQUE', 0), st.get('MULTIPLE', 0)))


def over600():
    """Describe nonosolve's webpbn puzzles that took more than 600 s."""
    names = dict(SURVEY + HARD)
    slow = [(names.get(stem(r['puzzle']), stem(r['puzzle'])), float(r['cpu']))
            for r in csv.DictReader(open(os.path.join(OURS, 'survey_ours.csv'))) if float(r['cpu']) > 600]
    if not slow:
        return ', and no puzzle needed more than 600 s'
    return ', and only %s needed more than 600 s' % ', '.join('%s (%.0f s)' % x for x in sorted(slow))


def main():
    os.makedirs(OUT, exist_ok=True)
    md = ['# Benchmark results', '',
          'All numbers were measured on one server (2x Intel Xeon Gold 5416S, Ubuntu 20.04, gcc 9.4), one process per '
          'physical core pinned with `taskset`, at most 8 jobs at a time.  Times are CPU seconds (user + system).', '',
          '* **webpbn puzzles**: find up to two solutions, i.e. solve and check uniqueness.  Competitors had 600 s; '
          'nonosolve had 3600 s%s.  `n/a` = puzzle too large for the '
          'solver, `err` = the solver rejected the input.' % over600(),
          '* **Tournament sets** (TAAI/TCGA/ICGA, 1000 puzzles of 25x25 each): first solution of every puzzle, in '
          'order, with the tournament limit of 2 hours per set.',
          '* **Random 30x30 puzzles** (Wolter): solve and check uniqueness, 120 s limit.', '',
          'Files: `webpbn.csv`, `webpbn_seeds.csv`, `tournament_sets.csv`, `tournament_puzzles.csv` (per-puzzle times), `rand30.csv`.', '',
          '## webpbn puzzles']
    webpbn(md)
    md.append('\n## Tournament sets: nonosolve vs LalaFrogKK')
    tournament(md)
    md.append('\n## 5000 random 30x30 puzzles')
    rand30(md)
    open(os.path.join(OUT, 'README.md'), 'w').write('\n'.join(md) + '\n')


if __name__ == '__main__':
    main()
