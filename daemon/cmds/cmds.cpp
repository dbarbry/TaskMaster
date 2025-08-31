#include "cmds.hpp"

extern std::map<std::string, int> active_programs;

std::string handle_cmd(std::string cmd, int server_fd, int client_fd,
                       std::map<std::string, std::vector<std::string>> parsedCommand,
                       TaskmasterConfig&                               config) {
    std::istringstream       iss(cmd);
    std::vector<std::string> words;
    std::string              word;
    std::ostringstream       response;

    while (iss >> word) words.push_back(word);

    if (words.empty()) {
        response << "Bad format, type help" << std::endl;
        return response.str();
    }

    const std::string& command = words[0];
    if (command == "reread")
        response << rereadCommand(config);
    else if (command == "update")
        response << updateCommand(config);
    else if (command == "status")
        response << statusCommand(parsedCommand, config.programs);
    else if (command == "start")
        response << startCommand(parsedCommand, config.programs);
    else if (command == "stop")
        response << stopCommand(parsedCommand, config.programs);
    else if (command == "restart")
        response << restartCommand(parsedCommand, config.programs);
    else if (command == "reload")
        response << reloadCommand(parsedCommand, config.programs, config);
    else
        response << "Command " + command + " not found." << std::endl << "Type 'help' for help.";
    response << std::endl;

    return response.str();
}
