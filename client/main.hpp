#ifndef MAIN_HPP
#define MAIN_HPP

#ifdef __APPLE__
    #include <util.h>
#elif defined(linux)
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
    std::string parse_cmd(const std::string &cmd) {
        std::istringstream iss(cmd);
        std::ostringstream oss;
        std::string        word;
        bool               first = true;

        while (iss >> word) {
            if (!first) oss << " ";
            oss << word;
            first = false;
        }

        return oss.str();
    }

    std::vector<std::string> cmd_to_words(const std::string &cmd) {
        std::istringstream       iss(cmd);
        std::vector<std::string> words;
        std::string              word;

        while (iss >> word) {
            words.push_back(word);
        }

        return words;
    }

    bool send_cmd(int fd, const std::string &cmd) {
        char        buffer[BUFFER_SIZE] = {0};
        std::string message             = cmd + "\n";
        ssize_t     bytes_read;

        if (write(fd, message.c_str(), message.size()) <= 0) {
            perror("write failed");
            return false;
        }

        bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read < 0) {
            perror("read failed");
            return false;
        } else if (bytes_read == 0) {
            std::cerr << "Server closed the connection.\n";
            return false;
        }

        std::cout << "Server: " << buffer;
        return true;
    }

    int attach_pty(int fd, const std::string &service_name) {
        std::string     message = "attach " + service_name + "\n";
        struct msghdr   msg;
        struct iovec    iov;
        char            buf[1] = {0};
        struct cmsghdr *cmsg;
        char            control[CMSG_SPACE(sizeof(int))];

        if (write(fd, message.c_str(), message.size()) <= 0) {
            perror("write failed");
            return -1;
        }
        memset(&msg, 0, sizeof(msg));

        std::cout << "[CLIENT] Requesting PTY for service: " << service_name << std::endl;

        iov.iov_base       = buf;
        iov.iov_len        = sizeof(buf);
        msg.msg_iov        = &iov;
        msg.msg_iovlen     = 1;
        msg.msg_control    = control;
        msg.msg_controllen = sizeof(control);

        if (recvmsg(fd, &msg, 0) <= 0) {
            perror("recvmsg failed");
            return -1;
        }

        cmsg = CMSG_FIRSTHDR(&msg);
        if (!cmsg || cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
            std::cerr << "Error: Invalid PTY descriptor received.\n";
            return -1;
        }

        int pty_fd = *((int *)CMSG_DATA(cmsg));
        std::cout << "Attached to " << service_name << std::endl;
        std::cout << "'detach' to leave" << std::endl;

        struct termios old_tio, new_tio;
        tcgetattr(STDIN_FILENO, &old_tio);
        new_tio = old_tio;
        new_tio.c_lflag |= ICANON | ECHO;
        // &= ~(ICANON | ECHO) to process raw buffer (1 char = 1 buffer)
        new_tio.c_cc[VQUIT] = _POSIX_VDISABLE;
        tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

        char buffer[1024];
        while (true) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            FD_SET(pty_fd, &fds);

            select(pty_fd + 1, &fds, nullptr, nullptr, nullptr);

            if (FD_ISSET(STDIN_FILENO, &fds)) {
                ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));

                if (n <= 0) break;
                buffer[n] = '\0';
                std::string input(buffer);

                if (input == "detach\n") {
                    std::cout << "\n[CLIENT] Detaching from " << service_name << "...\n";
                    break;
                }
                if (write(pty_fd, buffer, n) <= 0) {
                    perror("[CLIENT] write to PTY failed");
                    break;
                }
            }
            if (FD_ISSET(pty_fd, &fds)) {
                ssize_t n = read(pty_fd, buffer, sizeof(buffer));
                if (n <= 0) break;
                if (write(STDOUT_FILENO, buffer, n) <= 0) {
                    perror("[CLIENT] write to stdout failed");
                    break;
                }
            }
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
        close(pty_fd);
        return 0;
    }

    bool analyze_cmd(int fd, const std::string &cmd) {
        std::string              cleaned_cmd = parse_cmd(cmd);
        std::vector<std::string> words       = cmd_to_words(cleaned_cmd);

        if (words.empty()) return true;
        const std::string &command = words[0];

        if (cmd == "help") {
            std::cout << "Server commands:" << std::endl;
            std::cout << "  status all - get the status of all services" << std::endl;
            std::cout << "  status <serviceName> - get the status of a service" << std::endl;
            std::cout << "  start <serviceName> - start a service" << std::endl;
            std::cout << "  stop <serviceName> - stop a service" << std::endl;
            std::cout << "  restart <serviceName> - restart a service" << std::endl;
            std::cout << "  reload <pathToConfigFile> - reload the configfile" << std::endl;
            std::cout << "  shutdown - shutdown taskmaster server" << std::endl;
            std::cout << std::endl << "Client commands:" << std::endl;
            std::cout << "  exit - exit the client" << std::endl << std::endl;
            return true;
        } else if (cmd == "exit") {
            std::cout << "Leaving..." << std::endl;
            return false;
        } else if (command == "attach") {
            if (words.size() < 2) {
                std::cout << "Usage: attach <service>" << std::endl;
                return true;
            }
            if (attach_pty(fd, words[1]) != 0) std::cout << "Attach failed" << std::endl;
            return true;
        }

        if (!send_cmd(fd, cmd)) {
            std::cout << "Daemon crashed" << std::endl;
            return false;
        }
        return true;
    }
};

#endif
