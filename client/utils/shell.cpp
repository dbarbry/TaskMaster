#include "utils.hpp"

void flush_socket(int fd) {
    char dummy[256];
    int  flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0) {
        perror("fcntl F_GETFL");
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl F_SETFL");
        return;
    }
    while (true) {
        ssize_t n = read(fd, dummy, sizeof(dummy));
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else {
                perror("read error in flush_socket");
                break;
            }
        } else if (n == 0) {
            break;
        }
    }
    if (fcntl(fd, F_SETFL, flags) < 0) {
        perror("fcntl F_SETFL restore");
    }
}

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

std::vector<std::string> split_cmd(const std::string &cmd) {
    std::istringstream       iss(cmd);
    std::vector<std::string> words;
    std::string              word;

    while (iss >> word) {
        words.push_back(word);
    }

    return words;
}
