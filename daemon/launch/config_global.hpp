#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <grp.h>
#include <pwd.h>
#include <sys/resource.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

#include "config_program.hpp"
#include "logger.hpp"

class TaskmasterConfig {
   public:
    // [unix_http_server] section
    std::string                file  = "/tmp/taskmaster_socket";
    mode_t                     chmod = 0700;
    std::optional<std::string> chown;

   private:
    // if chown not set, we take current user privileges (not in taskmasterd.conf file)
    std::optional<uid_t> socket_uid;
    std::optional<gid_t> socket_gid;

   public:
    // [supervisord] section
    std::string                        logfile  = "/tmp/taskmaster.log";
    mode_t                             umask    = 022;
    bool                               nodaemon = false;
    bool                               silent   = false;
    int                                minfds   = 1024;
    int                                minprocs = 200;
    std::optional<std::string>         user;
    std::optional<std::string>         directory;
    std::map<std::string, std::string> environment;

    // [include] section
    std::string files = "/etc/supervisor/conf.d/*.conf";

    // [programs] and global datas
    std::map<std::string, ProgramConfig> programs;
    std::vector<std::string>             files_saved;

    // private data
    std::optional<uid_t> get_socket_uid() const { return socket_uid; }
    std::optional<gid_t> get_socket_gid() const { return socket_gid; }

   public:
    void set_socket_uid_gid(uid_t uid, gid_t gid) {
        if (uid == 0 && getuid() != 0)
            throw std::runtime_error("Only root can assign socket UID 0 (root)");
        socket_uid = uid;
        socket_gid = gid;
    }
};

TaskmasterConfig parse_taskmaster_conf(const std::string &filepath);

#endif  // CONFIG_HPP