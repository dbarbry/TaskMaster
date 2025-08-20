#include "../cmds.hpp"

extern std::map<std::string, int> attached_fds;

std::string detach(std::vector<std::string> words) {
    std::string service_name = words[1];
    auto        it           = attached_fds.find(service_name);
    int         fd_to_close  = it->second;

    if (words.size() < 2) return "Usage: detach <service_name>\n";

    if (it == attached_fds.end())
        return "Error: No active attached session for service " + service_name + "\n";

    if (close(fd_to_close) == -1)
        return "Error closing attached FD for service " + service_name + "\n";

    attached_fds.erase(it);
    return "Detached service " + service_name + "\n";
}
