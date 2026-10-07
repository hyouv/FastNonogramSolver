#!/usr/bin/env python3
"""Run nonogram solvers on puzzle files and record CPU time / outcome.

usage: run_bench.py --solvers ours,pbnsolve,... --timeout 300 --jobs 3 \
                    --out results.csv puzzles/survey/*.nin
Each puzzle is given as a .nin path; other formats are taken from puzzles/fmt.
"""
import argparse, csv, os, re, subprocess, sys, tempfile, threading, time
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SS = os.path.join(ROOT, 'third_party', 'survey_solvers')
FMT = os.path.join(ROOT, 'puzzles', 'fmt')


def dims(nin):
    w, h = map(int, open(nin).readline().split())
    return w, h


def fmt_path(nin, ext):
    return os.path.join(FMT, os.path.splitext(os.path.basename(nin))[0] + ext)


def command(solver, nin, timeout):
    """Return (argv, stdin_path) or None if the solver cannot handle the puzzle."""
    w, h = dims(nin)
    if solver == 'ours':
        return [os.path.join(ROOT, 'bin', 'nonosolve'), '-q', '--timeout', str(timeout), nin], None
    if solver.startswith('ours:'):  # ours with extra flags, e.g. "ours:--enc 1 --seed 2"
        return [os.path.join(ROOT, 'bin', 'nonosolve'), '-q', '--timeout', str(timeout)] + solver[5:].split() + [nin], None
    if solver == 'ours_nosat':
        return [os.path.join(ROOT, 'bin', 'nonosolve'), '-q', '--sat', 'none', '--timeout', str(timeout), nin], None
    if solver == 'pbnsolve':
        return [os.path.join(SS, 'pbnsolve', 'pbnsolve-1.09', 'pbnsolve'), '-u', nin], None
    if solver == 'naughty':
        if max(w, h) > 63:
            return None
        return [os.path.join(SS, 'naughty', 'naughty-v88', 'naughty63'), '-u'], fmt_path(nin, '.non')
    if solver == 'jsolver':
        return [os.path.join(SS, 'jsolver', 'jsolver-1.4-src', 'jsolver'), '-n', '2', fmt_path(nin, '.syro')], None
    if solver == 'grid':
        return [os.path.join(SS, 'grid', 'grid', 'grid'), '-total', '2', '-log', '1', fmt_path(nin, '.g')], None
    if solver == 'nonogrid_sat':
        return [os.path.join(ROOT, 'third_party', 'tsionyx_nonogrid', 'nonogrid_sat'), nin,
                '--timeout=%d' % timeout, '--max-solutions=2'], None
    if solver == 'nonogrid_bt':
        return [os.path.join(ROOT, 'third_party', 'tsionyx_nonogrid', 'nonogrid_bt'), nin,
                '--timeout=%d' % timeout, '--max-solutions=2'], None
    raise ValueError(solver)


def outcome(solver, text):
    t = text
    if solver.startswith('ours'):
        m = re.search(r' (UNIQUE|MULTIPLE|NONE|TIMEOUT|ERROR) ', t)
        return m.group(1) if m else '?'
    if solver == 'pbnsolve':
        if 'MULTIPLE' in t: return 'MULTIPLE'
        if 'UNIQUE' in t: return 'UNIQUE'
        if 'STALLED' in t or 'TIMEOUT' in t.upper(): return 'TIMEOUT'
        return '?'
    if solver == 'naughty':
        m = re.search(r'(\d+) solutions?', t)
        if m: return 'MULTIPLE' if int(m.group(1)) >= 2 else 'UNIQUE' if int(m.group(1)) == 1 else 'NONE'
        return '?'
    if solver == 'grid':
        m = re.search(r'number of solutions is (\d+)', t)
        if m: return 'MULTIPLE' if int(m.group(1)) >= 2 else 'UNIQUE'
        if 'more than' in t or 'at least 2' in t: return 'MULTIPLE'
        return '?'
    if solver == 'jsolver':
        m = re.findall(r'[Ss]olution[s]?\D*(\d+)', t)
        return m[-1] if m else '?'
    if solver.startswith('nonogrid'):
        if 'Backtracking' in t or 'solutions' in t: return 'done'
        return '?'
    return '?'


CPU_POOL = None  # queue of CPU ids to pin jobs to (taskset)


def run_one(solver, nin, timeout):
    cmd = command(solver, nin, timeout)
    if cmd is None:
        return dict(solver=solver, puzzle=os.path.basename(nin), cpu='', wall='', status='N/A', note='size')
    argv, stdin_path = cmd
    cpu_id = CPU_POOL.get() if CPU_POOL is not None else None
    if cpu_id is not None:
        argv = ['taskset', '-c', str(cpu_id)] + argv
    out = tempfile.TemporaryFile(mode='w+b')
    stdin = open(stdin_path, 'rb') if stdin_path else subprocess.DEVNULL
    t0 = time.time()
    p = subprocess.Popen(argv, stdin=stdin, stdout=out, stderr=subprocess.STDOUT,
                         cwd=os.path.dirname(argv[0]) if solver == 'lalafrog' else None)
    killed = {'v': False}

    def kill():
        killed['v'] = True
        try:
            p.kill()
        except Exception:
            pass

    timer = threading.Timer(timeout * 1.05 + 2, kill)
    timer.start()
    _, status, ru = os.wait4(p.pid, 0)
    timer.cancel()
    p.returncode = status
    if cpu_id is not None:
        CPU_POOL.put(cpu_id)
    wall = time.time() - t0
    cpu = ru.ru_utime + ru.ru_stime
    out.seek(0)
    text = out.read().decode('latin-1', 'replace')
    st = 'TIMEOUT' if killed['v'] or cpu > timeout else outcome(solver, text)
    tail = ' | '.join(l.strip() for l in text.strip().split('\n')[-3:])[:200]
    return dict(solver=solver, puzzle=os.path.basename(nin), cpu='%.3f' % cpu, wall='%.3f' % wall,
                status=st, note=tail)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--solvers', required=True)
    ap.add_argument('--timeout', type=float, default=300)
    ap.add_argument('--jobs', type=int, default=1)
    ap.add_argument('--out', required=True)
    ap.add_argument('--cpus', default='', help='comma separated CPU ids to pin jobs to')
    ap.add_argument('puzzles', nargs='+')
    a = ap.parse_args()
    global CPU_POOL
    if a.cpus:
        import queue
        CPU_POOL = queue.Queue()
        for c in a.cpus.split(','):
            CPU_POOL.put(int(c))
        a.jobs = min(a.jobs, CPU_POOL.qsize())
    tasks = [(s, p) for p in a.puzzles for s in a.solvers.split(',')]
    exists = set()
    if os.path.exists(a.out):
        for r in csv.DictReader(open(a.out)):
            exists.add((r['solver'], r['puzzle']))
    tasks = [t for t in tasks if (t[0], os.path.basename(t[1])) not in exists]
    lock = threading.Lock()
    new = not os.path.exists(a.out)
    f = open(a.out, 'a', newline='')
    wr = csv.DictWriter(f, fieldnames=['solver', 'puzzle', 'cpu', 'wall', 'status', 'note'])
    if new:
        wr.writeheader()

    def job(t):
        r = run_one(t[0], t[1], a.timeout)
        with lock:
            wr.writerow(r)
            f.flush()
            print('%-13s %-22s %10s %s' % (r['solver'], r['puzzle'], r['cpu'], r['status']), flush=True)

    with ThreadPoolExecutor(a.jobs) as ex:
        list(ex.map(job, tasks))


if __name__ == '__main__':
    main()
