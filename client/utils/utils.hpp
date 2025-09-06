#ifndef UTILS_HPP
#define UTILS_HPP

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

void setup_signal_handlers(void);

void                     flush_socket(int fd);
std::string              parse_cmd(const std::string &cmd);
std::vector<std::string> split_cmd(const std::string &cmd);

#endif
