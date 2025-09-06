#include "cmds.hpp"

bool isAlreadyRunning(const std::string &programPath) {
    std::string programName = basename(const_cast<char *>(programPath.c_str()));
    std::string command     = "pgrep -f " + programName + " > /dev/null";
    int         result      = system(command.c_str());
    return (result == 0);
}
