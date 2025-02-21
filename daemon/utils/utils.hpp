#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <unistd.h>
#include "../incs/parsing.hpp"

std::string handle_cmd(std::string cmd, int server_fd, int client_fd, std::map<std::string, std::vector<std::string>> parsedCommand, std::map<std::string, ProgramConfig> programs)
std::string handle_cmd(std::string cmd, int server_fd, int client_fd);

#endif
