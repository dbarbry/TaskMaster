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
#include "launch/launch.hpp"

const std::string& getConfigPath();


#endif
