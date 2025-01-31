#include <unistd.h>

#include <iostream>
#include <sstream>
#include <vector>

std::string status(std::vector<std::string> words) {
    words.clear();
    return "status command";
}

std::string start(std::vector<std::string> words) {
    words.clear();
    return "start command";
}

std::string stop(std::vector<std::string> words) {
    words.clear();
    return "stop command";
}

std::string restart(std::vector<std::string> words) {
    words.clear();
    return "restart command";
}

std::string reload(std::vector<std::string> words) {
    words.clear();
    return "reload command";
}

std::string shutdown(std::vector<std::string> words, int server_fd) {
    words.clear();
    close(server_fd);
    return "Shutting down daemon...";
}

std::string handle_cmd(std::string cmd, int server_fd) {
    std::istringstream       iss(cmd);
    std::vector<std::string> words;
    std::string              word;
    std::ostringstream       response;

    while (iss >> word) words.push_back(word);

    if (words.empty()) {
        response << "Bad format, type help" << std::endl;
        return response.str();
    }

    const std::string &command = words[0];
    if (command == "status")
        response << status(words);
    else if (command == "start")
        response << start(words);
    else if (command == "stop")
        response << stop(words);
    else if (command == "restart")
        response << restart(words);
    else if (command == "reload")
        response << reload(words);
    else if (command == "shutdown")
        response << shutdown(words, server_fd);
    else
        response << "Command " + command + " not found." << std::endl << "Type 'help' for help.";
    response << std::endl;

    return response.str();
}
