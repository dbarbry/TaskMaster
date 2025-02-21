#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include "../incs/start_command.hpp"
#include "../incs/stop_command.hpp"

extern std::map<std::string, int> active_programs;

std::string status(std::vector<std::string> words) {
    words.clear();
    return "status command";
}

std::string start(std::vector<std::string> words) {
    words.clear();
    return "start command";
}

std::string stop(std::vector<std::string> words) {
    words.clear();
    return "stop command";
}

std::string restart(std::vector<std::string> words) {
    words.clear();
    return "restart command";
}

std::string reload(std::vector<std::string> words) {
    words.clear();
    return "reload command";
}

std::string shutdown(std::vector<std::string> words, int server_fd) {
    words.clear();
    close(server_fd);
    return "Shutting down daemon...";
}

std::string attach(std::vector<std::string> words, int client_fd) {
    struct msghdr   msg;
    struct iovec    iov;
    char            buf[1] = {0};
    struct cmsghdr *cmsg;
    char            control[CMSG_SPACE(sizeof(int))];
    int             master_fd;
    int             dup_master;

    if (words.size() < 2) return "Usage: attach <service_name>\n";

    memset(&msg, 0, sizeof(msg));
    memset(control, 0, sizeof(control));
    std::string program_name = words[1];

    if (active_programs.find(program_name) == active_programs.end()) {
        return "Error: Program '" + program_name + "' not found or not running.\n";
    }

    master_fd = active_programs[program_name];
    std::cout << "[SERVER] Attaching to service: " << program_name << " (PTY FD: " << master_fd
              << ")" << std::endl;
    if (master_fd < 0) {
        return "Error: Invalid PTY file descriptor.\n";
    }

    dup_master = dup(master_fd);
    if (dup_master < 0) return "Error: Failed to duplicate PTY descriptor.\n";
    std::cout << "[SERVER] Attaching to service: " << program_name << " (PTY FD DUP: " << dup_master
              << ")" << std::endl;

    iov.iov_base       = buf;
    iov.iov_len        = sizeof(buf);
    msg.msg_iov        = &iov;
    msg.msg_iovlen     = 1;
    msg.msg_control    = control;
    msg.msg_controllen = sizeof(control);

    cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg) return "Error: Failed to allocate message header.\n";

    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(int));

    *((int *)CMSG_DATA(cmsg)) = dup_master;

    if (sendmsg(client_fd, &msg, 0) == -1) return "Error: Failed to send PTY descriptor.\n";

    return "";
}

std::string handle_cmd(std::string cmd, int server_fd, int client_fd, std::map<std::string, std::vector<std::string>> parsedCommand, std::map<std::string, ProgramConfig> programs) {
    std::istringstream       iss(cmd);
    std::vector<std::string> words;
    std::string              word;
    std::ostringstream       response;

    while (iss >> word) words.push_back(word);

    if (words.empty()) {
        response << "Bad format, type help" << std::endl;
        return response.str();
    }

    const std::string &command = words[0];
    if (command == "status")
        response << status(words);
    else if (command == "start") {
        response << start(words);
        startCommand(parsedCommand, programs);
    }
    else if (command == "stop")
    {
        response << stop(words);
        closeCommand(parsedCommand, programs);
    }
    else if (command == "restart")
        response << restart(words);
    else if (command == "reload")
        response << reload(words);
    else if (command == "shutdown")
        response << shutdown(words, server_fd);
    else if (command == "attach")
        response << attach(words, client_fd);
    else
        response << "Command " + command + " not found." << std::endl << "Type 'help' for help.";
    response << std::endl;

    return response.str();
}
