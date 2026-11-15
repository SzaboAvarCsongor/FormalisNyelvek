#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "rgx.hpp"

namespace {

// Common line prefix: date time pid tid level
// (the pattern suggested in the assignment, anchored at the start of the line)
const std::string P = "^[0-9-]+ [0-9:.]+ +[0-9]+ +[0-9]+ ";

std::vector<std::string> readLines(const std::string &path) {
    std::vector<std::string> lines;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

void writeLines(const std::string &path, const std::vector<std::string> &out) {
    std::ofstream f(path);
    for (size_t i = 0; i < out.size(); i++) {
        if (i) f << '\n';
        f << out[i];
    }
}

// 1. subtask group: count matching lines for each pattern
std::vector<std::string> task1(const std::vector<std::string> &lines) {
    const std::vector<std::regex> pats = {
        // 1. lines from 10-15
        std::regex("^10-15 "),
        // 2. 10-15 10:18:51
        std::regex("^10-15 10:18:51\\."),
        // 3. seconds == 51
        std::regex("^[0-9-]+ [0-9]+:[0-9]+:51\\."),
        // 4. lowmemorykiller tag
        std::regex(P + "[IDVEFAW] lowmemorykiller *:"),
        // 5. 10:18:51 with pid 221
        std::regex("^[0-9-]+ 10:18:51\\.[0-9]+ +221 +[0-9]+ [IDVEFAW] "),
        // 6. errors
        std::regex(P + "E "),
        // 7. PackageManager warnings
        std::regex(P + "W PackageManager *:"),
        // 8. ExoPlayer debug
        std::regex(P + "D [^:]*ExoPlayer[^:]*:"),
        // 9. errors and warnings with .java:
        std::regex(P + "[EW] .*\\.java:"),
        // 10. "Thread" in the message part (after the tag)
        std::regex(P + "[IDVEFAW] [^:]+: .*Thread"),
    };

    std::vector<int> cnt(pats.size(), 0);
    for (const auto &line : lines) {
        for (size_t i = 0; i < pats.size(); i++) {
            if (std::regex_search(line, pats[i])) cnt[i]++;
        }
    }

    std::vector<std::string> out;
    for (int c : cnt) out.push_back(std::to_string(c));
    return out;
}

// 2. file names (*.java) from error lines, in order of occurrence
std::vector<std::string> task2(const std::vector<std::string> &lines) {
    const std::regex err(P + "E ");
    const std::regex java("[A-Za-z0-9_$]+\\.java");

    std::vector<std::string> out;
    for (const auto &line : lines) {
        if (!std::regex_search(line, err)) continue;
        for (std::sregex_iterator it(line.begin(), line.end(), java), end; it != end; ++it) {
            out.push_back(it->str());
        }
    }
    return out;
}

// 3. distinct process ids in order of first appearance
std::vector<std::string> task3(const std::vector<std::string> &lines) {
    const std::regex re(P.substr(0, 0) + "^[0-9-]+ [0-9:.]+ +([0-9]+) +[0-9]+ [IDVEFAW] ");
    std::set<std::string> seen;
    std::vector<std::string> out;
    std::smatch m;
    for (const auto &line : lines) {
        if (std::regex_search(line, m, re)) {
            std::string pid = m[1].str();
            if (seen.insert(pid).second) out.push_back(pid);
        }
    }
    return out;
}

// 4. process with the most errors
std::vector<std::string> task4(const std::vector<std::string> &lines) {
    const std::regex re("^[0-9-]+ [0-9:.]+ +([0-9]+) +[0-9]+ E ");
    std::map<std::string, int> cnt;
    std::vector<std::string> order;
    std::smatch m;
    for (const auto &line : lines) {
        if (std::regex_search(line, m, re)) {
            std::string pid = m[1].str();
            if (cnt.find(pid) == cnt.end()) order.push_back(pid);
            cnt[pid]++;
        }
    }
    std::vector<std::string> out;
    if (order.empty()) return out;
    std::string best = order[0];
    for (const auto &pid : order) {
        if (cnt[pid] > cnt[best]) best = pid; // tie: the earlier one stays
    }
    out.push_back("PID=" + best + " (" + std::to_string(cnt[best]) + " errors)");
    return out;
}

// 5. errors on the main thread (pid == tid)
std::vector<std::string> task5(const std::vector<std::string> &lines) {
    const std::regex re("^[0-9-]+ [0-9:.]+ +([0-9]+) +\\1 E ");
    int count = 0;
    for (const auto &line : lines) {
        if (std::regex_search(line, re)) count++;
    }
    return {std::to_string(count)};
}

} // namespace

void RgxProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("rgx", "Regex task number", cxxopts::value<std::string>());
}

bool RgxProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("rgx") > 0;
}

int RgxProblem::run(const cxxopts::ParseResult &args) {
    std::string inPath = args["input"].as<std::string>();
    std::string outPath = args["output"].as<std::string>();
    std::string task = args["rgx"].as<std::string>();

    std::ifstream probe(inPath);
    if (!probe) {
        std::cerr << "Cannot open input file" << std::endl;
        return 1;
    }
    probe.close();

    std::vector<std::string> lines = readLines(inPath);
    std::vector<std::string> out;

    if (task == "1") out = task1(lines);
    else if (task == "2") out = task2(lines);
    else if (task == "3") out = task3(lines);
    else if (task == "4") out = task4(lines);
    else if (task == "5") out = task5(lines);
    else {
        std::cerr << "Unknown task number: " << task << std::endl;
        return 1;
    }

    writeLines(outPath, out);
    return 0;
}