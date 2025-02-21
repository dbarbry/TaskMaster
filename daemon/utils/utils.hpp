#ifndef UTILS_HPP
#define UTILS_HPP

#include <libgen.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../incs/parsing.hpp"

std::string handle_cmd(std::string cmd, int server_fd, int client_fd,
                       std::map<std::string, std::vector<std::string>> parsedCommand,
                       std::map<std::string, ProgramConfig>            programs);
bool        isAlreadyRunning(const std::string &programPath);

#endif
