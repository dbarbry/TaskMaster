#ifndef UTILS_HPP
#define UTILS_HPP

#include <libgen.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "../cmds/service_types.hpp"
#include "../launch/config_global.hpp"
#include "../launch/config_program.hpp"
#include "./service_state.hpp"

std::string restartCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                           const std::map<std::string, ProgramConfig>            &programs);

std::string handle_cmd(std::string cmd, int server_fd, int client_fd,
                       std::map<std::string, std::vector<std::string>> parsedCommand,
                       TaskmasterConfig                               &config);

std::map<std::string, std::vector<std::string>> commandParsing(std::string cmd);

std::string startCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                         const std::map<std::string, ProgramConfig>            &programs);
std::string stopCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                        const std::map<std::string, ProgramConfig>            &programs);
std::string attachCommand(std::vector<std::string> words, int client_fd);

std::string statusCommand(const std::map<std::string, std::vector<std::string>> &cmd,
                          const std::map<std::string, ProgramConfig>            &programs);
std::string rereadCommand(TaskmasterConfig &config);
std::string updateCommand(TaskmasterConfig &config);
bool        isAlreadyRunning(const std::string &programPath);

#endif  // UTILS_HPP
