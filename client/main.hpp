#ifndef MAIN_HPP
#define MAIN_HPP

#ifdef __APPLE__
#include <util.h>
#else
#include <pty.h>
#endif

#include <fcntl.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <termios.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "utils/utils.hpp"
#define BUFFER_SIZE 1024

class Shell {
   public:
    void run(int fd) {
        char *input;

        while (true) {
            input = readline("\033[34mtaskmaster\033[0m$ ");
            if (!input) {  // ctrl D
                std::cout << "Leaving..." << std::endl;
                break;
            }

            std::string cmd(input);
            free(input);

            if (!cmd.empty()) {
                add_history(cmd.c_str());
                std::string clean_cmd = parse_cmd(cmd);

                if (!analyze_cmd(fd, clean_cmd)) {
                    break;
                }
            }
        }
    }

   private:
    bool send_cmd(int fd, const std::string &cmd) {
        std::string message = cmd + "\n";
        std::string response;
        char        buffer[BUFFER_SIZE];
        ssize_t     bytes_read;

        if (write(fd, message.c_str(), message.size()) <= 0) {
            perror("write failed");
            return false;
        }

        while (true) {
            bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
            if (bytes_read < 0) {
                perror("read failed");
                return false;
            } else if (bytes_read == 0) {
                break;
            }

            buffer[bytes_read] = '\0';
            response += buffer;

            // end of transmission
            if (bytes_read < BUFFER_SIZE - 1) break;
        }

        std::cout << response;
        if (response.empty()) {
            std::cerr << "Server closed the connection.\n";
            return false;
        }

        if (!response.empty() && response.back() != '\n') std::cout << std::endl;
        return true;
    }

    bool analyze_cmd(int fd, const std::string &cmd) {
        std::string              cleaned_cmd = parse_cmd(cmd);
        std::vector<std::string> words       = split_cmd(cleaned_cmd);

        if (words.empty()) return true;
        const std::string &command = words[0];

        if (cmd == "help") {
            std::cout << "Server commands:" << std::endl;
            std::cout << "  status - get the status of all services" << std::endl;
            std::cout << "  status <serviceName> - get the status of a service" << std::endl;
            std::cout << "  start <serviceName> - start a service" << std::endl;
            std::cout << "  stop <serviceName> - stop a service" << std::endl;
            std::cout << "  reload - reload the config file" << std::endl;
            std::cout << "  reread - read for added or deleted programs" << std::endl;
            std::cout << "  update - update added or deleted programs" << std::endl;
            std::cout << "  restart <serviceName> - restart a service" << std::endl;
            std::cout << std::endl << "Client commands:" << std::endl;
            std::cout << "  exit - exit the client" << std::endl << std::endl;
            return true;
        } else if (cmd == "exit") {
            std::cout << "Leaving..." << std::endl;
            return false;
        }

        if (!send_cmd(fd, cmd)) {
            std::cout << "Daemon crashed" << std::endl;
            return false;
        }
        return true;
    }
};

#endif
