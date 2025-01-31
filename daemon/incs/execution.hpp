#ifndef EXECUTION_HPP
#define EXECUTION_HPP

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include "./parsing.hpp"

void exec_programs(const std::map<std::string, ProgramConfig> &programs);

#endif