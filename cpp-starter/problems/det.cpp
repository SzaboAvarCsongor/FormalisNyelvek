#include "det.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <queue>
#include <sstream>
#include <vector>

void DetProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("det", "Determinize the automaton");
}

bool DetProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("det") > 0;
}

bool DetProblem::readAutomaton() {
    std::ifstream inputFile(inputFilename);

    if (!inputFile) {
        std::cerr << "Cannot open input file: "
                  << inputFilename << '\n';
        return false;
    }

    std::string line;
    std::string token;

    // 1. sor: állapotok
    if (!std::getline(inputFile, line)) {
        std::cerr << "Invalid input file.\n";
        return false;
    }

    {
        std::stringstream ss(line);
        while (ss >> token) {
            states.insert(token);
        }
    }

    // 2. sor: ábécé
    if (!std::getline(inputFile, line)) {
        std::cerr << "Invalid input file.\n";
        return false;
    }

    {
        std::stringstream ss(line);
        while (ss >> token) {
            alphabet.push_back(token);
        }
    }

    // Az ábécé sorrendje legyen lexikografikus.
    std::sort(alphabet.begin(), alphabet.end());
    alphabet.erase(
        std::unique(alphabet.begin(), alphabet.end()),
        alphabet.end()
    );

    // kezdőállapot
    if (!std::getline(inputFile, line)) {
        std::cerr << "Invalid input file.\n";
        return false;
    }

    {
        std::stringstream ss(line);
        if (!(ss >> startState) || !states.count(startState)) {
            std::cerr << "Invalid start state.\n";
            return false;
        }
    }

    // végállapotok
    if (!std::getline(inputFile, line)) {
        std::cerr << "Invalid input file.\n";
        return false;
    }

    {
        std::stringstream ss(line);
        while (ss >> token) {
            if (!states.count(token)) {
                std::cerr << "Invalid final state: " << token << '\n';
                return false;
            }
            finalStates.insert(token);
        }
    }

    // A további sorok: forrásállapot, szimbólum, célállapot
    while (std::getline(inputFile, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string from, symbol, to;

        if (!(ss >> from >> symbol >> to)) {
            std::cerr << "Invalid transition line: " << line << '\n';
            return false;
        }

        if (!states.count(from) || !states.count(to)) {
            std::cerr << "Transition contains an unknown state.\n";
            return false;
        }

        if (std::find(alphabet.begin(), alphabet.end(), symbol)
            == alphabet.end()) {
            std::cerr << "Transition contains an unknown symbol: "
                      << symbol << '\n';
            return false;
        }

        transitions[from][symbol].insert(to);
    }

    return true;
}

bool DetProblem::determinize() {
    using StateSet = std::set<std::string>;

    std::vector<StateSet> dfaStates;
    std::map<StateSet, int> stateNumbers;
    std::queue<StateSet> pending;

    StateSet initial = {startState};

    dfaStates.push_back(initial);
    stateNumbers[initial] = 0;
    pending.push(initial);

    while (!pending.empty()) {
        StateSet current = pending.front();
        pending.pop();

        for (const std::string& symbol : alphabet) {
            StateSet destination;

            for (const std::string& state : current) {
                auto stateIt = transitions.find(state);

                if (stateIt == transitions.end()) {
                    continue;
                }

                auto symbolIt = stateIt->second.find(symbol);

                if (symbolIt == stateIt->second.end()) {
                    continue;
                }

                destination.insert(
                    symbolIt->second.begin(),
                    symbolIt->second.end()
                );
            }

            if (destination.empty()) {
                continue;
            }

            if (!stateNumbers.count(destination)) {
                int number = static_cast<int>(dfaStates.size());

                stateNumbers[destination] = number;
                dfaStates.push_back(destination);
                pending.push(destination);
            }
        }
    }

    std::ofstream outputFile(outputFilename);

    if (!outputFile) {
        std::cerr << "Cannot open output file: "
                  << outputFilename << '\n';
        return false;
    }

    // 1. sor: s0 s1 s2 ...
    for (int i = 0; i < static_cast<int>(dfaStates.size()); ++i) {
        if (i > 0) outputFile << ' ';
        outputFile << 's' << i;
    }
    outputFile << '\n';

    // 2. sor: ábécé
    for (std::size_t i = 0; i < alphabet.size(); ++i) {
        if (i > 0) outputFile << ' ';
        outputFile << alphabet[i];
    }
    outputFile << '\n';

    // 3. sor: kezdőállapot
    outputFile << "s0\n";

    // 4. sor: végállapotok
    bool first = true;

    for (int i = 0; i < static_cast<int>(dfaStates.size()); ++i) {
        bool isFinal = false;

        for (const std::string& state : dfaStates[i]) {
            if (finalStates.count(state)) {
                isFinal = true;
                break;
            }
        }

        if (isFinal) {
            if (!first) outputFile << ' ';
            outputFile << 's' << i;
            first = false;
        }
    }
    outputFile << '\n';

    // Átmenetek: állapotsorrend, majd ábécésorrend.
    for (int i = 0; i < static_cast<int>(dfaStates.size()); ++i) {
        for (const std::string& symbol : alphabet) {
            StateSet destination;

            for (const std::string& state : dfaStates[i]) {
                auto stateIt = transitions.find(state);

                if (stateIt == transitions.end()) {
                    continue;
                }

                auto symbolIt = stateIt->second.find(symbol);

                if (symbolIt != stateIt->second.end()) {
                    destination.insert(
                        symbolIt->second.begin(),
                        symbolIt->second.end()
                    );
                }
            }

            if (!destination.empty()) {
                outputFile << 's' << i << ' '
                           << symbol << ' '
                           << 's' << stateNumbers.at(destination)
                           << '\n';
            }
        }
    }

    return true;
}

int DetProblem::run(const cxxopts::ParseResult &args) {
    inputFilename = args["input"].as<std::string>();
    outputFilename = args["output"].as<std::string>();

    if (!readAutomaton()) {
        return 1;
    }

    if (!determinize()) {
        return 1;
    }

    return 0;
}