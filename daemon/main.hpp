#ifndef MAIN_HPP
#define MAIN_HPP

#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <thread>

#include "cmds/cmds.hpp"
#include "launch/config_program.hpp"

const std::string& getConfigPath();
void               daemonize(TaskmasterConfig& config);
void               run_server(TaskmasterConfig& config);

#endif  // MAIN_HPP
