#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <glob.h>
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
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

#include "config_program.hpp"
#include "logger.hpp"

namespace utils {
std::vector<std::string> expand_glob(const std::string& pattern);
std::string              to_lower_copy(const std::string& input);
static std::string       trim(const std::string& line);
}  // namespace utils

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
    // config file path
    std::string conf_path;

    // [supervisord] section
    std::string                        logfile  = "/tmp/taskmaster.log";
    mode_t                             umask    = 022;
    bool                               nodaemon = false;
    bool                               silent   = false;
    std::optional<std::string>         pidfile  = "/tmp/taskmasterd.pid";
    int                                minfds   = 1024;
    int                                minprocs = 200;
    std::optional<std::string>         user;
    std::optional<std::string>         directory;
    std::map<std::string, std::string> environment;

    // [include] section
    std::string files = "/etc/supervisor/conf.d/*.conf";

    // [programs] and global datas
    std::map<std::string, ProgramConfig> programs;

    // private data
    std::optional<uid_t> get_socket_uid() const { return socket_uid; }
    std::optional<gid_t> get_socket_gid() const { return socket_gid; }

    // lists used in reread
    std::vector<std::string> program_to_stop;
    std::vector<std::string> program_to_start;

   public:
    void set_socket_uid_gid(uid_t uid, gid_t gid) {
        if (uid == 0 && getuid() != 0)
            throw std::runtime_error("Only root can assign socket UID 0 (root)");
        socket_uid = uid;
        socket_gid = gid;
    }

    std::vector<std::string> get_current_program_names(const TaskmasterConfig& config) {
        std::vector<std::string> program_names;

        for (const auto& [name, _] : config.programs) program_names.push_back(name);

        return program_names;
    }

    std::vector<std::string> get_programs_names_in_files(const std::string& pattern) {
        std::vector<std::string> program_names;
        glob_t                   glob_result;
        int                      ret = glob(pattern.c_str(), GLOB_TILDE, nullptr, &glob_result);

        if (ret != 0) {
            std::cerr << "Error reading pattern: " << pattern << std::endl;
            globfree(&glob_result);
            return program_names;
        }

        for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
            std::string filepath = glob_result.gl_pathv[i];

            std::map<std::string, ProgramConfig> parsed_programs = parsing(filepath);

            for (const auto& [name, _] : parsed_programs) {
                program_names.push_back(name);
            }
        }

        globfree(&glob_result);
        return program_names;
    }
};

TaskmasterConfig parse_taskmaster_conf(const std::string& filepath);

#endif  // CONFIG_HPP