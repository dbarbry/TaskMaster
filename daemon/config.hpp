#include <grp.h>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include "logger.hpp"
#include "main.hpp"

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

    // for the two privates values
    std::optional<uid_t> get_socket_uid() const { return socket_uid; }
    std::optional<gid_t> get_socket_gid() const { return socket_gid; }

    void set_socket_uid_gid(uid_t uid, gid_t gid) {
        if (uid == 0 && getuid() != 0)
            throw std::runtime_error("Only root can assign socket UID 0 (root)");
        socket_uid = uid;
        socket_gid = gid;
    }
};

namespace config_parser {

using KeyValue = std::pair<std::string, std::string>;

static std::string trim(const std::string &line) {
    auto start = line.find_first_not_of(" \t");
    auto end   = line.find_last_not_of(" \t");

    return (start == std::string::npos) ? "" : line.substr(start, end - start + 1);
}

static bool is_comment_or_empty(const std::string &line) {
    std::string trimmed = trim(line);

    return trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#';
}

static std::map<std::string, std::string> parse_environment(const std::string &line) {
    std::map<std::string, std::string> env_map;
    std::regex                         env_regex(R"(([^=]+)=\"([^\"]*)\")");
    auto begin = std::sregex_iterator(line.begin(), line.end(), env_regex);
    auto end   = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::smatch match = *it;
        if (match.size() == 3) env_map[trim(match[1])] = match[2];
    }
    return env_map;
}

static std::optional<KeyValue> parse_line(const std::string &raw_line) {
    auto        comment_pos = raw_line.find(';');
    std::string clean_line =
        (comment_pos != std::string::npos) ? raw_line.substr(0, comment_pos) : raw_line;
    auto equal_pos = clean_line.find('=');
    if (equal_pos == std::string::npos) return std::nullopt;
    return std::make_pair(trim(clean_line.substr(0, equal_pos)),
                          trim(clean_line.substr(equal_pos + 1)));
}

static void parse_chown(const std::string &chown_str, std::optional<uid_t> &uid_out,
                        std::optional<gid_t> &gid_out) {
    auto        colon_pos = chown_str.find(':');
    std::string user_part =
        (colon_pos != std::string::npos) ? chown_str.substr(0, colon_pos) : chown_str;
    std::string group_part =
        (colon_pos != std::string::npos) ? chown_str.substr(colon_pos + 1) : "";

    struct passwd *pw = getpwnam(user_part.c_str());
    if (!pw) throw std::runtime_error("Invalid user in chown: " + user_part);
    uid_out = pw->pw_uid;

    if (!group_part.empty()) {
        struct group *gr = getgrnam(group_part.c_str());
        if (!gr) throw std::runtime_error("Invalid group in chown: " + group_part);
        gid_out = gr->gr_gid;
    } else {
        gid_out = pw->pw_gid;
    }
}

int parse_integer(const std::string &value, const std::string &field_name) {
    try {
        int intValue = std::stoi(value);
        return intValue;
    } catch (const std::invalid_argument &e) {
        Logger::error("Error parsing '" + field_name + "' with value '" + value +
                      "': Invalid integer format.");
        throw std::runtime_error("Invalid integer format for '" + field_name + "': " + value);
    } catch (const std::out_of_range &e) {
        Logger::error("Error parsing '" + field_name + "' with value '" + value +
                      "': Integer value out of range.");
        throw std::runtime_error("Integer value out of range for '" + field_name + "': " + value);
    }
}

}  // namespace config_parser

namespace config_validator {

void validate_umask(mode_t umask) {
    if (umask > 0777) throw std::invalid_argument("Invalid umask: must be <= 0777");
}

void validate_minfds(int value) {
    if (value < 0) throw std::runtime_error("minfds must be non-negative");
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) != 0)
        throw std::runtime_error("Failed to get RLIMIT_NOFILE");

    if ((rlim_t)value > limit.rlim_cur) {
        limit.rlim_cur = value;
        if (setrlimit(RLIMIT_NOFILE, &limit) != 0)
            throw std::runtime_error("Failed to raise RLIMIT_NOFILE to required minfds");
    }
    if (value < 256) std::cerr << "Warning: minfds is low; consider using at least 1024.\n";
}

void validate_minprocs(int value) {
    struct rlimit limit;

    if (value < 0) throw std::runtime_error("minprocs must be non-negative");
    if (getrlimit(RLIMIT_NPROC, &limit) != 0)
        throw std::runtime_error("Failed to get RLIMIT_NPROC");

    if ((rlim_t)value > limit.rlim_cur) {
        limit.rlim_cur = value;
        if (setrlimit(RLIMIT_NPROC, &limit) != 0)
            throw std::runtime_error("Failed to raise RLIMIT_NPROC to required minprocs");
    }
    if (value < 50) std::cerr << "Warning: minprocs is low; consider using at least 200.\n";
}

}  // namespace config_validator

TaskmasterConfig parse_taskmaster_conf(const std::string &filepath) {
    TaskmasterConfig      config;
    std::ifstream         file(filepath);
    std::string           line;
    std::string           current_section;
    std::string           lower_key;
    std::optional<uid_t>  uid;
    std::optional<gid_t>  gid;
    std::filesystem::path config_dir;

    if (!file.is_open()) throw std::runtime_error("Unable to open config file: " + filepath);

    config_dir           = std::filesystem::absolute(std::filesystem::path(filepath)).parent_path();
    auto substitute_here = [&](const std::string &val) -> std::string {
        std::string result = val;
        size_t      pos    = 0;
        while ((pos = result.find("%(here)s", pos)) != std::string::npos) {
            result.replace(pos, 8, config_dir.string());
            pos += config_dir.string().size();
        }
        return result;
    };

    while (std::getline(file, line)) {
        line = config_parser::trim(line);
        if (config_parser::is_comment_or_empty(line)) continue;

        if (line.front() == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            std::transform(current_section.begin(), current_section.end(), current_section.begin(),
                           ::tolower);
            continue;
        }

        auto kv_opt = config_parser::parse_line(line);
        if (!kv_opt) continue;

        const auto &[key, value] = *kv_opt;
        lower_key                = key;
        std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);

        if (current_section == "unix_http_server") {
            if (lower_key == "file")
                config.file = value;
            else if (lower_key == "chmod")
                config.chmod = std::stoul(value, nullptr, 8);
            else if (lower_key == "chown") {
                config.chown = value;
                try {
                    config_parser::parse_chown(value, uid, gid);
                    config.set_socket_uid_gid(*uid, *gid);
                } catch (const std::exception &e) {
                    throw std::runtime_error("Failed to parse chown value: " +
                                             std::string(e.what()));
                }
            }
        } else if (current_section == "supervisord") {
            if (lower_key == "logfile")
                config.logfile = value;
            else if (lower_key == "umask") {
                config.umask = std::stoul(value, nullptr, 8);
                config_validator::validate_umask(config.umask);
            } else if (lower_key == "nodaemon")
                config.nodaemon = (value == "true");
            else if (lower_key == "silent")
                config.silent = (value == "true");
            else if (lower_key == "minfds") {
                config.minfds = config_parser::parse_integer(lower_key, "minfds");
                config_validator::validate_minfds(config.minfds);
            } else if (lower_key == "minprocs") {
                config.minprocs = config_parser::parse_integer(lower_key, "minprocs");
                config_validator::validate_minprocs(config.minprocs);
            } else if (lower_key == "user") {
                if (geteuid() != 0)
                    throw std::runtime_error(
                        "The 'user' directive may only be used when running as root.");
                config.user = value;
            } else if (lower_key == "directory") {
                std::filesystem::path dir_path(value);
                if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path))
                    throw std::runtime_error(
                        "Invalid directory: does not exist or is not a directory: " + value);
                config.directory = value;
            } else if (lower_key == "environment")
                config.environment = config_parser::parse_environment(value);
        }
    }

    return config;
}
