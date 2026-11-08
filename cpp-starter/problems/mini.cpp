#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "mini.hpp"

namespace {

bool allDigits(const std::string &s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; });
}

// Name ordering: q2 < q10 (numeric part compared as a number), fallback: plain string order
bool nameLess(const std::string &a, const std::string &b) {
    size_t pa = a.find_first_of("0123456789");
    size_t pb = b.find_first_of("0123456789");
    if (pa != std::string::npos && pb != std::string::npos) {
        std::string da = a.substr(pa), db = b.substr(pb);
        if (allDigits(da) && allDigits(db)) {
            da.erase(0, std::min(da.find_first_not_of('0'), da.size() - 1));
            db.erase(0, std::min(db.find_first_not_of('0'), db.size() - 1));
            if (da.size() != db.size()) return da.size() < db.size();
            if (da != db) return da < db;
        }
    }
    return a < b;
}

// q1 -> s1
std::string newName(const std::string &name) {
    size_t p = name.find_first_of("0123456789");
    return "s" + (p != std::string::npos ? name.substr(p) : name);
}

bool readLine(std::ifstream &in, std::string &line) {
    if (!std::getline(in, line)) return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return true;
}

std::vector<std::string> split(const std::string &line) {
    std::vector<std::string> out;
    std::stringstream ss(line);
    std::string t;
    while (ss >> t) out.push_back(t);
    return out;
}

} // namespace

void MiniProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("mini", "Minimize the DFA");
}

bool MiniProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("mini") > 0;
}

int MiniProblem::run(const cxxopts::ParseResult &args) {
    std::ifstream in(args["input"].as<std::string>());
    if (!in) {
        std::cerr << "Cannot open input file" << std::endl;
        return 1;
    }

    // ---------- reading ----------
    std::string line;
    readLine(in, line);
    std::vector<std::string> stateList = split(line);
    readLine(in, line);
    std::vector<std::string> alphabet = split(line);
    readLine(in, line);
    std::vector<std::string> tmp = split(line);
    std::string start = tmp.empty() ? "" : tmp[0];
    readLine(in, line);
    std::set<std::string> finals;
    for (auto &s : split(line)) finals.insert(s);

    std::set<std::string> stateSet(stateList.begin(), stateList.end());
    std::set<std::string> symSet(alphabet.begin(), alphabet.end());

    std::map<std::string, std::map<std::string, std::string>> trans;
    while (readLine(in, line)) {
        std::vector<std::string> t = split(line);
        if (t.empty()) continue;
        if (t.size() != 3 || !stateSet.count(t[0]) || !symSet.count(t[1]) || !stateSet.count(t[2])) {
            std::cerr << "Invalid transition: " << line << std::endl;
            return 1;
        }
        trans[t[0]][t[1]] = t[2];
    }
    if (!stateSet.count(start)) {
        std::cerr << "Invalid start state" << std::endl;
        return 1;
    }
    for (auto &f : finals) {
        if (!stateSet.count(f)) {
            std::cerr << "Invalid final state: " << f << std::endl;
            return 1;
        }
    }

    // ---------- reachable states ----------
    std::set<std::string> reach;
    std::vector<std::string> stack{start};
    reach.insert(start);
    while (!stack.empty()) {
        std::string s = stack.back();
        stack.pop_back();
        for (auto &kv : trans[s]) {
            if (reach.insert(kv.second).second) stack.push_back(kv.second);
        }
    }
    std::vector<std::string> st(reach.begin(), reach.end());
    std::sort(st.begin(), st.end(), nameLess);
    int n = (int)st.size();
    std::map<std::string, int> idx;
    for (int i = 0; i < n; i++) idx[st[i]] = i;
    int k = (int)alphabet.size();
    int dead = n; // virtual trap state for missing transitions

    std::vector<std::vector<int>> delta(n + 1, std::vector<int>(k, dead));
    for (int i = 0; i < n; i++) {
        for (int a = 0; a < k; a++) {
            auto it = trans[st[i]].find(alphabet[a]);
            if (it != trans[st[i]].end()) delta[i][a] = idx[it->second];
        }
    }

    // ---------- Moore partition refinement ----------
    std::vector<int> cls(n + 1, 0);
    for (int i = 0; i < n; i++) cls[i] = finals.count(st[i]) ? 1 : 0;
    int count = (int)std::set<int>(cls.begin(), cls.end()).size();
    while (true) {
        std::map<std::vector<int>, int> sigs;
        std::vector<int> next(n + 1);
        for (int i = 0; i <= n; i++) {
            std::vector<int> sig{cls[i]};
            for (int a = 0; a < k; a++) sig.push_back(cls[delta[i][a]]);
            auto it = sigs.find(sig);
            if (it == sigs.end()) it = sigs.emplace(sig, (int)sigs.size()).first;
            next[i] = it->second;
        }
        int newCount = (int)sigs.size();
        cls = next;
        if (newCount == count) break;
        count = newCount;
    }


    std::map<int, std::string> rep;
    for (int i = 0; i < n; i++) {
        auto it = rep.find(cls[i]);
        if (it == rep.end() || nameLess(st[i], it->second)) rep[cls[i]] = st[i];
    }
    std::vector<int> order; // classes ordered by their representative
    for (auto &kv : rep) order.push_back(kv.first);
    std::sort(order.begin(), order.end(),
              [&](int x, int y) { return nameLess(rep[x], rep[y]); });
    std::map<int, int> pos;
    for (int i = 0; i < (int)order.size(); i++) pos[order[i]] = i;

    std::set<int> finalCls;
    for (int i = 0; i < n; i++)
        if (finals.count(st[i])) finalCls.insert(pos[cls[i]]);

    std::set<std::tuple<int, std::string, int>> outTrans;
    for (int i = 0; i < n; i++) {
        for (int a = 0; a < k; a++) {
            auto it = trans[st[i]].find(alphabet[a]);
            if (it == trans[st[i]].end()) continue;
            outTrans.insert({pos[cls[i]], alphabet[a], pos[cls[idx[it->second]]]});
        }
    }

    // ---------- writing ----------
    auto nm = [&](int p) { return newName(rep[order[p]]); };

    std::ofstream out(args["output"].as<std::string>());
    for (size_t i = 0; i < order.size(); i++) out << (i ? " " : "") << nm((int)i);
    out << '\n';
    for (int a = 0; a < k; a++) out << (a ? " " : "") << alphabet[a];
    out << '\n';
    out << nm(pos[cls[idx[start]]]) << '\n';
    bool first = true;
    for (int f : finalCls) {
        out << (first ? "" : " ") << nm(f);
        first = false;
    }
    for (auto &t : outTrans) {
        out << '\n' << nm(std::get<0>(t)) << ' ' << std::get<1>(t) << ' ' << nm(std::get<2>(t));
    }
    return 0;
}