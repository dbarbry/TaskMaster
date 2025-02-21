#ifndef CLOSE_COMMAND_HPP
#define CLOSE_COMMAND_HPP

#include <libgen.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <libgen.h>

#include "utils/utils.hpp"
#include "parsing.hpp"

void closeCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                  const std::map<std::string, ProgramConfig>            &programs);
#endif