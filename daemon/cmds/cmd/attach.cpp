#include "../cmds.hpp"

extern std::map<std::string, int> active_programs;
extern std::map<std::string, int> attached_fds;

std::string attachCommand(std::vector<std::string> words, int client_fd) {
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
    std::string service_name = words[1];

    if (active_programs.find(service_name) == active_programs.end()) {
        return "Error: Program '" + service_name + "' not found or not running.\n";
    }

    master_fd = active_programs[service_name];
    std::cout << "[SERVER] Attaching to service: " << service_name << " (PTY FD: " << master_fd
              << ")" << std::endl;
    if (master_fd < 0) return "Error: Invalid PTY file descriptor.\n";

    if (attached_fds.find(service_name) != attached_fds.end()) {
        close(attached_fds[service_name]);
        attached_fds.erase(service_name);
    }

    dup_master = dup(master_fd);
    if (dup_master < 0) return "Error: Failed to duplicate PTY descriptor.\n";

    attached_fds[service_name] = dup_master;
    std::cout << "[SERVER] Attaching to service: " << service_name << " (PTY FD DUP: " << dup_master
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
    close(dup_master);

    return "";
}
