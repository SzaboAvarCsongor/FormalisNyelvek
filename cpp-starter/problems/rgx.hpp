#ifndef RGX_HPP
#define RGX_HPP

#include "../problem.hpp"

class RgxProblem : public Problem {
public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;
};

#endif // RGX_HPP