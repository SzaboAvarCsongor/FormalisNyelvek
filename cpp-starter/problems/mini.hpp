#ifndef MINI_HPP
#define MINI_HPP

#include <string>
#include "../problem.hpp"


#include <map>
#include <set>
#include <string>
#include <vector>

class MiniProblem : public Problem {
public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;
};

#endif // MINI_HPP