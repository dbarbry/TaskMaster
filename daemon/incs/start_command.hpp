#ifndef START_COMMAND_HPP
#define START_COMMAND_HPP

#include <libgen.h>
#include <sys/wait.h>
#include <unistd.h>
#include "utils/utils.hpp"
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include "parsing.hpp"


void startCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                  const std::map<std::string, ProgramConfig> &programs);

bool isAlreadyRunning(const std::string &programPath);

#endif