// Puzzle representation and file parsers.
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct Puzzle {
    int H = 0, W = 0;                    // rows, columns
    std::vector<std::vector<int>> rows;  // H clues, each left to right
    std::vector<std::vector<int>> cols;  // W clues, each top to bottom
    std::string name;

    bool sane(std::string* why = nullptr) const {
        auto fail = [&](const char* m) {
            if (why) *why = m;
            return false;
        };
        if (H <= 0 || W <= 0) return fail("empty puzzle");
        if ((int)rows.size() != H || (int)cols.size() != W) return fail("clue count mismatch");
        long sr = 0, sc = 0;
        for (auto& r : rows) {
            long need = -1;
            for (int x : r) {
                if (x <= 0) return fail("non-positive clue");
                need += x + 1;
                sr += x;
            }
            if (need > W) return fail("row clue too long");
        }
        for (auto& c : cols) {
            long need = -1;
            for (int x : c) {
                if (x <= 0) return fail("non-positive clue");
                need += x + 1;
                sc += x;
            }
            if (need > H) return fail("column clue too long");
        }
        if (sr != sc) return fail("row/column totals differ");
        return true;
    }
};

namespace parse {

static inline std::vector<int> nums(const std::string& s) {
    std::vector<int> v;
    const char* p = s.c_str();
    while (*p) {
        while (*p && !(*p >= '0' && *p <= '9')) p++;
        if (!*p) break;
        int x = 0;
        while (*p >= '0' && *p <= '9') x = x * 10 + (*p++ - '0');
        if (x > 0) v.push_back(x);
    }
    return v;
}

static inline std::vector<std::string> lines_of(const std::string& text) {
    std::vector<std::string> out;
    std::string cur;
    for (char ch : text) {
        if (ch == '\n') {
            out.push_back(cur);
            cur.clear();
        } else if (ch != '\r')
            cur += ch;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

static inline bool blank(const std::string& s) {
    for (char c : s)
        if (!isspace((unsigned char)c)) return false;
    return true;
}

// .nin: "W H", then H row clue lines, then W column clue lines ("0" = empty).
static inline bool nin(const std::string& text, Puzzle& p) {
    auto L = lines_of(text);
    size_t i = 0;
    while (i < L.size() && blank(L[i])) i++;
    if (i >= L.size()) return false;
    auto hw = nums(L[i++]);
    if (hw.size() < 2) return false;
    p.W = hw[0];
    p.H = hw[1];
    std::vector<std::vector<int>> all;
    for (; i < L.size() && (int)all.size() < p.H + p.W; i++) {
        if (blank(L[i])) continue;
        all.push_back(nums(L[i]));
    }
    if ((int)all.size() != p.H + p.W) return false;
    p.rows.assign(all.begin(), all.begin() + p.H);
    p.cols.assign(all.begin() + p.H, all.end());
    return true;
}

// .cwd: H, W, H row lines, blank line, W column lines (blank lines = empty clue).
static inline bool cwd(const std::string& text, Puzzle& p) {
    auto L = lines_of(text);
    size_t i = 0;
    while (i < L.size() && blank(L[i])) i++;
    if (i + 1 >= L.size()) return false;
    p.H = atoi(L[i++].c_str());
    p.W = atoi(L[i++].c_str());
    if (p.H <= 0 || p.W <= 0) return false;
    p.rows.clear();
    p.cols.clear();
    for (int r = 0; r < p.H; r++) {
        if (i >= L.size()) return false;
        p.rows.push_back(nums(L[i++]));
    }
    if (i < L.size() && blank(L[i])) i++;
    for (int c = 0; c < p.W; c++) {
        if (i >= L.size()) {
            p.cols.push_back({});
            continue;
        }
        p.cols.push_back(nums(L[i++]));
    }
    return true;
}

// Steve Simpson's .non format (keywords width/height/rows/columns).
static inline bool non(const std::string& text, Puzzle& p) {
    auto L = lines_of(text);
    p.H = p.W = 0;
    p.rows.clear();
    p.cols.clear();
    int mode = 0;
    for (auto& s : L) {
        std::istringstream is(s);
        std::string kw;
        is >> kw;
        if (kw == "width") {
            is >> p.W;
            mode = 0;
        } else if (kw == "height") {
            is >> p.H;
            mode = 0;
        } else if (kw == "rows") {
            mode = 1;
        } else if (kw == "columns") {
            mode = 2;
        } else if (mode && (kw.empty() || isdigit((unsigned char)kw[0]))) {
            if (kw.empty() && blank(s)) {
                // blank line terminates a section only when it is already full
                if (mode == 1 && (int)p.rows.size() == p.H) mode = 0;
                else if (mode == 2 && (int)p.cols.size() == p.W) mode = 0;
                else if (mode == 1) p.rows.push_back({});
                else p.cols.push_back({});
                continue;
            }
            if (mode == 1 && (int)p.rows.size() < p.H) p.rows.push_back(nums(s));
            else if (mode == 2 && (int)p.cols.size() < p.W) p.cols.push_back(nums(s));
        } else {
            mode = 0;
        }
    }
    return p.H > 0 && p.W > 0 && (int)p.rows.size() == p.H && (int)p.cols.size() == p.W;
}

// TAAI / TCGA tournament batch format: "$id" line, then N column clues
// (left to right) then N row clues (top to bottom).  Square puzzles.
static inline bool taai(const std::string& text, std::vector<Puzzle>& out, int N = 0) {
    auto L = lines_of(text);
    std::vector<size_t> starts;
    for (size_t i = 0; i < L.size(); i++)
        if (!L[i].empty() && L[i][0] == '$') starts.push_back(i);
    for (size_t s = 0; s < starts.size(); s++) {
        size_t b = starts[s] + 1, e = (s + 1 < starts.size()) ? starts[s + 1] : L.size();
        std::vector<std::vector<int>> all;
        for (size_t i = b; i < e; i++) all.push_back(nums(L[i]));
        int n = N;
        if (n == 0) {
            // drop trailing blank lines beyond an even count
            while (all.size() % 2 && !all.empty() && all.back().empty()) all.pop_back();
            n = (int)all.size() / 2;
        }
        while ((int)all.size() < 2 * n) all.push_back({});
        Puzzle p;
        p.H = p.W = n;
        p.cols.assign(all.begin(), all.begin() + n);
        p.rows.assign(all.begin() + n, all.begin() + 2 * n);
        p.name = L[starts[s]].substr(1);
        out.push_back(p);
    }
    return !out.empty();
}

static inline std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static inline std::string ext_of(const std::string& path) {
    auto d = path.rfind('.');
    if (d == std::string::npos) return "";
    return path.substr(d + 1);
}

static inline bool any_format(const std::string& path, const std::string& fmt, Puzzle& p) {
    std::string text = read_file(path);
    std::string f = fmt.empty() || fmt == "auto" ? ext_of(path) : fmt;
    p.name = path;
    if (f == "nin") return nin(text, p);
    if (f == "cwd") return cwd(text, p);
    if (f == "non" || f == "ss") return non(text, p);
    if (f == "taai" || f == "txt") {
        std::vector<Puzzle> v;
        if (!taai(text, v)) return false;
        p = v[0];
        return true;
    }
    // sniff
    if (text.find("width") != std::string::npos && text.find("rows") != std::string::npos)
        return non(text, p);
    if (text.find('$') != std::string::npos) {
        std::vector<Puzzle> v;
        if (!taai(text, v)) return false;
        p = v[0];
        return true;
    }
    return nin(text, p);
}

}  // namespace parse
