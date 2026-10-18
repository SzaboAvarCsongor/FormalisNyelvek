#ifndef PROBLEMS_DET_H
#define PROBLEMS_DET_H

#include "../problem.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

class DetProblem : public Problem {
private:
    std::vector<std::string> alphabet;
    std::string startState;
    std::set<std::string> states;
    std::set<std::string> finalStates;

    std::map<std::string,
        std::map<std::string, std::set<std::string>>> transitions;

    std::string inputFilename;
    std::string outputFilename;

    bool readAutomaton();
    bool determinize();

public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;
};

#endif