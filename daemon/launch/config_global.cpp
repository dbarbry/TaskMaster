#include "config_global.hpp"

struct ConfigSection {
    std::string                                      name;
    std::vector<std::pair<std::string, std::string>> key_values;
};

/**
 * @brief Parses the [unix_http_server] section of the configuration file.
 *
 * This function extracts and processes configuration keys specific to the
 * [unix_http_server] section such as `file`, `chmod`, and `chown`. Validates
 * the permissions and ownership and stores them in the TaskmasterConfig object.
 *
 * @param config Reference to the TaskmasterConfig structure to populate.
 * @param section The parsed key-value pairs from the [unix_http_server] section.
 */
void parse_unix_http_server(TaskmasterConfig &config, const ConfigSection &section) {
    for (const auto &[key, value] : section.key_values) {
        std::string lower_key = utils::to_lower_copy(key);

        if (lower_key == "file")
            config.file = config_validator::validate_path(value, lower_key);
        else if (lower_key == "chmod")
            config.chmod = config_validator::validate_octal(value, lower_key);
        else if (lower_key == "chown") {
            config.chown                 = value;
            auto [user_part, group_part] = config_parser::parse_chown(value);
            auto [uid, gid]              = config_validator::validate_chown(user_part, group_part);
            config.set_socket_uid_gid(uid, gid);
        }
    }
}

/**
 * @brief Parses the [taskmasterd] section of the configuration file.
 *
 * Extracts and validates key configuration values like `logfile`, `umask`,
 * `nodaemon`, `silent`, `minfds`, `minprocs`, `user`, `directory`, and
 * `environment`. Ensures constraints are enforced and sets the corresponding
 * fields in the TaskmasterConfig object.
 *
 * @param config Reference to the TaskmasterConfig structure to populate.
 * @param section The parsed key-value pairs from the [taskmasterd] section.
 */
void parse_taskmasterd(TaskmasterConfig &config, const ConfigSection &section) {
    for (const auto &[key, value] : section.key_values) {
        std::string lower_key = utils::to_lower_copy(key);

        if (lower_key == "logfile") {
            auto abs_path  = config_validator::validate_path(value, lower_key);
            config.logfile = config_validator::validate_folder(abs_path, lower_key);
        } else if (lower_key == "umask")
            config.umask = config_validator::validate_octal(value, lower_key);
        else if (lower_key == "nodaemon")
            config.nodaemon = config_validator::validate_bool(value, lower_key);
        else if (lower_key == "silent") {
            config.silent  = config_validator::validate_bool(value, lower_key);
            Logger::silent = config.silent;
        } else if (lower_key == "minfds") {
            config.minfds = config_validator::validate_integer(value, lower_key);
            config_validator::validate_minfds(config.minfds);
        } else if (lower_key == "minprocs") {
            config.minprocs = config_validator::validate_integer(value, lower_key);
            config_validator::validate_minprocs(config.minprocs);
        } else if (lower_key == "pidfile")
            config.pidfile = config_validator::validate_path(value, lower_key);
        else if (lower_key == "user")
            config.user = config_validator::validate_user_field(value, lower_key);
        else if (lower_key == "directory")
            config.directory = config_validator::validate_path(value, lower_key);
        else if (lower_key == "environment")
            config.environment = config_parser::parse_environment(value);
    }
}

/**
 * @brief Parses the [include] section of the configuration file.
 *
 * Extracts and validates the `files` directive, which specifies the absolute
 * path to a directory containing additional `.conf` files.
 *
 * @param config Reference to the TaskmasterConfig structure to populate.
 * @param section The parsed key-value pairs from the [include] section.
 */
void parse_include(TaskmasterConfig &config, const ConfigSection &section) {
    for (const auto &[key, value] : section.key_values) {
        std::string lower_key = utils::to_lower_copy(key);

        if (lower_key == "files") {
            std::string path = config_validator::validate_path(value, lower_key);

            if (path.find('*') == std::string::npos || !path.ends_with(".conf")) {
                throw std::runtime_error(
                    "The 'files' directive must be a glob ending in '*.conf': " + value);
            }

            config.files = path;
        }
    }
}

/**
 * @brief Parses the entire Taskmaster configuration file.
 *
 * Reads the given configuration file, splits it into sections, and
 * dispatches section-specific parsing functions. Returns a populated
 * TaskmasterConfig object. Unknown sections are ignored with a warning.
 *
 * @param filepath The path to the configuration file to parse.
 * @return A fully populated TaskmasterConfig object.
 *
 * @throws std::runtime_error If the file cannot be opened or contains
 *         invalid values or structure.
 */
TaskmasterConfig parse_taskmaster_conf(const std::string &filepath) {
    TaskmasterConfig                     config;
    std::ifstream                        file(filepath);
    std::map<std::string, ConfigSection> sections;
    std::string                          current_section;
    std::string                          line;
    int                                  check = 0;

    if (!file.is_open()) throw std::runtime_error("Unable to open config file: " + filepath);

    while (std::getline(file, line)) {
        line = utils::trim(line);
        if (utils::is_comment_or_empty(line)) continue;

        if (line.front() == '[' && line.back() == ']') {
            current_section           = line.substr(1, line.size() - 2);
            current_section           = utils::to_lower_copy(current_section);
            sections[current_section] = ConfigSection {current_section, {}};
            continue;
        } else if (line.front() == '[' && line.back() != ']' ||
                   line.front() != '[' && line.back() == ']') {
            Logger::error("Invalid program syntax: " + line);
            continue;
        }

        auto kv_opt = config_parser::parse_line(line);
        if (!kv_opt || current_section.empty()) continue;
        sections[current_section].key_values.emplace_back(*kv_opt);
    }

    try {
        config.conf_path = std::filesystem::absolute(filepath).lexically_normal().string();
    } catch (const std::exception &e) {
        throw std::runtime_error("Failed to resolve absolute path for config file: " + filepath +
                                 " (" + e.what() + ")");
    }
    for (auto &[section_name, section] : sections) {
        if (section_name == "unix_http_server") {
            parse_unix_http_server(config, section);
            check += 1;
        } else if (section_name == "taskmasterd") {
            parse_taskmasterd(config, section);
            check += 1;
        } else if (section_name == "include") {
            parse_include(config, section);
            check += 1;
        } else
            Logger::warn("Unknown section [" + section_name + "] is ignored.");
    }
    if (check != 3) {
        throw std::runtime_error(
            "Missing of of the three sections [unix_http_server], [taskmasterd] or [include] in "
            "main config file.");
    }

    return config;
}
