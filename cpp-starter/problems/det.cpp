#include "det.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <queue>
#include <sstream>
#include <vector>

// Inicializálja a determináláshoz szükséges parancssori kapcsolót.
void DetProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("det", "Determinize the automaton");
}

// Ellenőrzi, hogy a determinálás feladatot választották-e.
bool DetProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("det") > 0;
}

// Beolvassa és ellenőrzi az automata adatait a bemeneti fájlból.
bool DetProblem::readAutomaton() {
    std::ifstream inputFile(inputFilename);

    if (!inputFile) {
        std::cerr << "Cannot open input file: "
                  << inputFilename << '\n';
        return false;
    }

    std::string line;
    std::string token;

    // Beolvassa az állapotokat.
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

    // Beolvassa és rendezi az ábécét.
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

    // Beolvassa és ellenőrzi a kezdőállapotot.
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

    // Beolvassa és ellenőrzi a végállapotokat.
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

    // Beolvassa és ellenőrzi az átmeneteket.
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

// Determinisztikussá alakítja az automatát, majd kiírja az eredményt.
bool DetProblem::determinize() {
    using StateSet = std::set<std::string>;

    std::vector<StateSet> dfaStates;
    std::map<StateSet, int> stateNumbers;
    std::queue<StateSet> pending;

    StateSet initial = {startState};

    dfaStates.push_back(initial);
    stateNumbers[initial] = 0;
    pending.push(initial);

    // Megkeresi az elérhető állapothalmazokat.
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

    // Megnyitja a kimeneti fájlt.
    std::ofstream outputFile(outputFilename);

    if (!outputFile) {
        std::cerr << "Cannot open output file: "
                  << outputFilename << '\n';
        return false;
    }

    // Kiírja a determinisztikus automata állapotait.
    for (int i = 0; i < static_cast<int>(dfaStates.size()); ++i) {
        if (i > 0) outputFile << ' ';
        outputFile << 's' << i;
    }
    outputFile << '\n';

    // Kiírja az ábécét.
    for (std::size_t i = 0; i < alphabet.size(); ++i) {
        if (i > 0) outputFile << ' ';
        outputFile << alphabet[i];
    }
    outputFile << '\n';

    // Kiírja a kezdőállapotot.
    outputFile << "s0\n";

    // Kiírja azokat az állapotokat, amelyek végállapotok.
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

    // Kiírja a determinisztikus automata átmeneteit.
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

// Beolvassa a fájlneveket, majd elindítja a beolvasást és a determinálást.
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