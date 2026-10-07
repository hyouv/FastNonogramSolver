// nonosolve: fast nonogram solver.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "engine.h"
#include "puzzle.h"
#include "solver.h"

static void usage() {
    fprintf(stderr,
            "usage: nonosolve [options] puzzle-file\n"
            "       nonosolve --taai input [-o output] [--log logfile]\n"
            "options:\n"
            "  --format F     nin|cwd|non|taai (default: by extension)\n"
            "  -n N           stop after N solutions (default 2 = uniqueness check)\n"
            "  --timeout S    give up after S seconds\n"
            "  --sat MODE     none|kissat|cadical (default kissat)\n"
            "  --dfs-nodes N  DFS node budget before switching to CDCL\n"
            "  -q             do not print the solution grid\n"
            "  -v             verbose statistics\n");
}

int main(int argc, char** argv) {
    Options opt;
    std::string file, fmt, taaiIn, taaiOut = "solution.txt", taaiLog;
    bool quiet = false;
    std::string jsonOut;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                usage();
                exit(2);
            }
            return argv[++i];
        };
        if (a == "--format") fmt = next();
        else if (a == "-n") {
            opt.maxSolutions = atoi(next().c_str());
            opt.maxSolutionsSet = true;
        }
        else if (a == "--timeout") opt.timeout = atof(next().c_str());
        else if (a == "--sat") opt.sat = next();
        else if (a == "--dfs-nodes") opt.dfsNodes = atol(next().c_str());
        else if (a == "--dfs-work") opt.dfsWork = atof(next().c_str());
        else if (a == "--taai") taaiIn = next();
        else if (a == "--dimacs") opt.dimacs = next();
        else if (a == "--json") jsonOut = next();
        else if (a == "--branch") opt.branchHeur = atoi(next().c_str());
        else if (a == "--value") opt.valueOrder = atoi(next().c_str());
        else if (a == "--node-limit") opt.nodeLimit = atol(next().c_str());
        else if (a == "--enc") opt.encoding = atoi(next().c_str());
        else if (a == "--seed") opt.seed = atoi(next().c_str());
        else if (a == "--kissat-config") opt.kissatConfig = next();
        else if (a == "--interleave") opt.interleave = true;
        else if (a == "--probe-clauses") opt.probeClauses = true;
        else if (a == "--no-probe-clauses") opt.probeClauses = false;
        else if (a == "--dfs-work1") opt.dfsWork1 = atof(next().c_str());
        else if (a == "--sat-conflicts1") opt.satConflicts1 = atol(next().c_str());
        else if (a == "--cadical-conflicts") opt.cadicalConflicts = atoi(next().c_str());
        else if (a == "--cache-bits") opt.cacheBits = atoi(next().c_str());
        else if (a == "--restarts") opt.restartBase = atol(next().c_str());
        else if (a == "--noise") opt.noise = atof(next().c_str());
        else if (a == "-o") taaiOut = next();
        else if (a == "--log") taaiLog = next();
        else if (a == "-q") quiet = true;
        else if (a == "-v") opt.verbose = true;
        else if (a == "-h" || a == "--help") {
            usage();
            return 0;
        } else if (!a.empty() && a[0] == '-') {
            fprintf(stderr, "unknown option %s\n", a.c_str());
            usage();
            return 2;
        } else file = a;
    }

    if (!taaiIn.empty()) {
        std::vector<Puzzle> ps;
        if (!parse::taai(parse::read_file(taaiIn), ps)) {
            fprintf(stderr, "cannot parse %s\n", taaiIn.c_str());
            return 1;
        }
        if (opt.maxSolutions == 2 && !opt.maxSolutionsSet) opt.maxSolutions = 1;
        FILE* out = fopen(taaiOut.c_str(), "w");
        FILE* log = taaiLog.empty() ? nullptr : fopen(taaiLog.c_str(), "w");
        double t0 = now_sec();
        int solved = 0;
        for (size_t i = 0; i < ps.size(); i++) {
            double ts = now_sec();
            Result res = solvePuzzle(ps[i], opt);
            double te = now_sec() - ts;
            fprintf(out, "$%s\n", ps[i].name.c_str());
            for (int r = 0; r < ps[i].H; r++)
                for (int c = 0; c < ps[i].W; c++) {
                    int v = res.solutions.empty() ? 0 : res.solutions[0][r * ps[i].W + c];
                    fprintf(out, "%d%c", v, c + 1 == ps[i].W ? '\n' : '\t');
                }
            if (!res.solutions.empty()) solved++;
            if (log)
                fprintf(log, "#%s\t%.6f\t%s\tnodes=%ld probes=%ld lines=%ld hits=%ld root=%d sat=%d\n", ps[i].name.c_str(), te,
                        res.statusName(), res.stats.nodes, res.stats.probes, res.stats.lineSolves, res.stats.cacheHits,
                        res.stats.rootKnown, (int)res.usedSat);
        }
        double tt = now_sec() - t0;
        if (log) {
            fprintf(log, "total time: %f\n", tt);
            fclose(log);
        }
        fclose(out);
        printf("solved %d/%zu in %.3f s\n", solved, ps.size(), tt);
        return 0;
    }

    if (file.empty()) {
        usage();
        return 2;
    }
    Puzzle p;
    if (!parse::any_format(file, fmt, p)) {
        fprintf(stderr, "cannot parse %s\n", file.c_str());
        return 1;
    }
    double t0 = now_sec();
    Result res = solvePuzzle(p, opt);
    double tt = now_sec() - t0;
    printf("%s %dx%d %s time=%.4f nodes=%ld probes=%ld sat=%d\n", file.c_str(), p.W, p.H,
           res.statusName(), tt, res.stats.nodes, res.stats.probes, res.usedSat);
    if (!jsonOut.empty()) {
        FILE* f = fopen(jsonOut.c_str(), "w");
        auto clueList = [&](const std::vector<std::vector<int>>& v) {
            fputc('[', f);
            for (size_t i = 0; i < v.size(); i++) {
                fputc('[', f);
                for (size_t j = 0; j < v[i].size(); j++) fprintf(f, "%s%d", j ? "," : "", v[i][j]);
                fprintf(f, "]%s", i + 1 < v.size() ? "," : "");
            }
            fputc(']', f);
        };
        fprintf(f, "{\"file\":\"%s\",\"W\":%d,\"H\":%d,\"status\":\"%s\",\"time\":%.4f,\"nodes\":%ld,\"probes\":%ld,\"sat\":%d,\"rows\":",
                file.c_str(), p.W, p.H, res.statusName(), tt, res.stats.nodes, res.stats.probes, (int)res.usedSat);
        clueList(p.rows);
        fprintf(f, ",\"cols\":");
        clueList(p.cols);
        fprintf(f, ",\"solutions\":[");
        for (size_t k = 0; k < res.solutions.size(); k++) {
            fprintf(f, "%s\"", k ? "," : "");
            for (uint8_t v : res.solutions[k]) fputc(v ? '1' : '0', f);
            fputc('"', f);
        }
        fprintf(f, "]}\n");
        fclose(f);
    }
    if (res.solutions.size() >= 2) {
        int d = 0;
        for (size_t i = 0; i < res.solutions[0].size(); i++) d += res.solutions[0][i] != res.solutions[1][i];
        printf("solutions 1 and 2 differ in %d cells\n", d);
    }
    if (!quiet && !res.solutions.empty()) {
        for (int r = 0; r < p.H; r++) {
            for (int c = 0; c < p.W; c++) putchar(res.solutions[0][r * p.W + c] ? '#' : '.');
            putchar('\n');
        }
    }
    return 0;
}
