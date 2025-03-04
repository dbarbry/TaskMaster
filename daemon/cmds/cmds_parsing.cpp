#include "cmds.hpp"

std::map<std::string, std::vector<std::string>> commandParsing(std::string cmd) {
    std::istringstream                              iss(cmd);
    std::string                                     word;
    std::map<std::string, std::vector<std::string>> parsedCommand;

    if (iss >> word) {
        parsedCommand["command"].push_back(word);
    }

    while (iss >> word) {
        parsedCommand["args"].push_back(word);
    }

    std::cout << "[DEBUG] Command parsed: " << parsedCommand["command"][0] << std::endl;
    std::cout << "[DEBUG] Args: ";
    for (const auto &arg : parsedCommand["args"]) {
        std::cout << arg << " ";
    }
    std::cout << std::endl;

    return parsedCommand;
}
