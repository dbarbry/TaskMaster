#include "parsing.hpp"

namespace utils {

/**
 * @brief Remove leading and trailing whitespace from a string.
 *
 * @param line The input string to trim.
 * @return A new string with leading and trailing whitespace removed.
 */
std::string trim(const std::string &line) {
    auto start = line.find_first_not_of(" \t");
    auto end   = line.find_last_not_of(" \t");

    return (start == std::string::npos) ? "" : line.substr(start, end - start + 1);
}

/**
 * @brief Expand a glob pattern into a list of matching file paths.
 *
 * This function uses the POSIX glob() system call to resolve wildcard patterns 
 * like "*.conf" or "configs/*.conf" into a vector of actual file paths.
 *
 * @param pattern The glob pattern to match files against (supports wildcards like '*', '?', etc.).
 * @return A vector of strings, each representing a file path that matches the pattern.
 *
 * @note If the pattern does not match any files, an empty vector is returned.
 * @warning If glob fails (e.g., due to bad syntax or memory allocation), an error is logged to stderr.
 */
std::vector<std::string> expand_glob(const std::string &pattern) {
    std::vector<std::string> results;
    glob_t                   glob_result;

    int ret = glob(pattern.c_str(), GLOB_TILDE, nullptr, &glob_result);
    if (ret != 0) {
        Logger::error("Glob failed for pattern: " + pattern);
        globfree(&glob_result);
        return results;
    }

    for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
        results.emplace_back(glob_result.gl_pathv[i]);
    }

    globfree(&glob_result);
    return results;
}

/**
 * @brief Create a lowercase copy of a string.
 *
 * @param input The input string to convert.
 * @return A new string where all alphabetic characters are lowercase.
 */
std::string to_lower_copy(const std::string &input) {
    std::string result = input;

    std::transform(result.begin(), result.end(), result.begin(), ::tolower);

    return result;
}

/**
 * @brief Create a uppercase copy of a string.
 *
 * @param input The input string to convert.
 * @return A new string where all alphabetic characters are uppercase.
 */
std::string to_upper_copy(const std::string &input) {
    std::string result = input;

    std::transform(result.begin(), result.end(), result.begin(), ::toupper);

    return result;
}

/**
 * @brief Determine whether a line is a comment or empty after trimming.
 *
 * Comments are defined as lines that start with '#' or ';'.
 *
 * @param line The input line to evaluate.
 * @return true if the line is empty or a comment, false otherwise.
 */
bool is_comment_or_empty(const std::string &line) {
    const std::string trimmed = utils::trim(line);
    return trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#';
}

}  // namespace utils

namespace config_parser {

/**
 * @brief Parse a config line into a key-value pair.
 *
 * This function strips comments starting with ';', trims spaces around
 * key and value, and validates presence of '=' separator.
 *
 * @param raw_line The raw line from config file.
 * @return Optional KeyValue pair if parse succeeded, std::nullopt otherwise.
 */
std::optional<KeyValue> parse_line(const std::string &raw_line) {
    const auto        comment_pos = raw_line.find(';');
    const std::string clean_line =
        (comment_pos != std::string::npos) ? raw_line.substr(0, comment_pos) : raw_line;
    const auto equal_pos = clean_line.find('=');

    if (equal_pos == std::string::npos) {
        return std::nullopt;
    }

    const std::string key   = utils::trim(clean_line.substr(0, equal_pos));
    const std::string value = utils::trim(clean_line.substr(equal_pos + 1));

    if (key.empty() || value.empty())
        throw std::runtime_error("Empty value for '" + key + "' in configuration file");

    return KeyValue {key, value};
}

/**
 * @brief Parse a chown string of format "user:group" or "user".
 *
 * @param chown_str The chown string.
 * @return Pair of user part and group part (group part may be empty).
 */
std::pair<std::string, std::string> parse_chown(const std::string &chown_str) {
    size_t      colon_pos = chown_str.find(':');
    std::string user_part =
        (colon_pos != std::string::npos) ? chown_str.substr(0, colon_pos) : chown_str;
    std::string group_part =
        (colon_pos != std::string::npos) ? chown_str.substr(colon_pos + 1) : "";

    return {user_part, group_part};
}

/**
 * @brief Parse environment variables from a string of format:
 * key1="value1" key2="value2" ...
 *
 * @param line The environment line string.
 * @return Map of key-value environment pairs.
 */
std::map<std::string, std::string> parse_environment(const std::string &line) {
    std::map<std::string, std::string> env_map;
    const std::regex                   env_regex(R"(([^= \t]+)=\"([^\"]*)\")");

    auto begin = std::sregex_iterator(line.begin(), line.end(), env_regex);
    auto end   = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::smatch match = *it;
        if (match.size() == 3) {
            std::string key = utils::trim(match[1].str());
            std::string val = match[2].str();
            if (!key.empty()) {
                env_map[key] = val;  // overwrite duplicate, do we keep this ?
            }
        }
    }

    return env_map;
}

/**
 * @brief Parse commands variables from a string of format:
 *
 * @param line The environment line string.
 * @return Map of key-value environment pairs.
 */
std::string parse_command(const std::string &line) {
    std::string trimmed = utils::trim(line);

    return trimmed;
}

}  // namespace config_parser

namespace config_validator {

/**
 * @brief Validate that a given string is an absolute filesystem path.
 *
 * @param value The path string to validate.
 * @param field_name The name of the config field (used in error messages).
 * @return The validated absolute path.
 * @throws std::runtime_error if the path is not absolute.
 */
std::filesystem::path validate_path(const std::string &value, const std::string &field_name) {
    std::filesystem::path path(value);

    if (!path.is_absolute()) {
        throw std::runtime_error("Invalid path for " + field_name + ": must be absolute (" + value +
                                 ")");
    }

    return path;
}

/**
 * @brief Validate that a given absolute path is a directory (folder).
 *
 * @param path The absolute path to validate.
 * @param field_name The name of the config field (for error messages).
 * @return The validated path (unchanged) if it's a folder.
 * @throws std::runtime_error if the path is not a folder or does not exist.
 */
std::filesystem::path validate_folder(const std::filesystem::path &path,
                                      const std::string           &field_name) {
    if (!std::filesystem::is_directory(path) || !std::filesystem::exists(path)) {
        throw std::runtime_error("" + field_name + " must be a folder, not a file (" +
                                 path.string() + ")");
    }

    return path;
}

/**
 * @brief Validate that a given string is an absolute, existing directory path.
 *
 * @param value The directory path string to validate.
 * @param field_name The name of the config field (used in error messages).
 * @return The validated directory path.
 * @throws std::runtime_error if the path is not absolute or not a directory.
 */
std::filesystem::path validate_dir(const std::string &value, const std::string &field_name) {
    std::filesystem::path path = validate_path(value, field_name);

    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        throw std::runtime_error("Invalid directory for " + field_name +
                                 ": does not exist or is not a directory (" + value + ")");
    }

    return path;
}

/**
 * @brief Validate that a given string is an absolute path to an existing regular file.
 *
 * @param value The file path string to validate.
 * @param field_name The name of the config field (used in error messages).
 * @return The validated file path.
 * @throws std::runtime_error if the path is not absolute or not a regular file.
 */
std::filesystem::path validate_file_exists(const std::string &value,
                                           const std::string &field_name) {
    std::filesystem::path path = validate_path(value, field_name);

    if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("Invalid file for " + field_name +
                                 ": does not exist or is not a regular file (" + value + ")");
    }

    return path;
}

/**
 * @brief Validate that a string represents a valid integer.
 *
 * @param value The string to parse.
 * @param field_name The name of the config field (used in error messages).
 * @return The parsed integer value.
 * @throws std::runtime_error if the format is invalid or out of range.
 */
int validate_integer(const std::string &value, const std::string &field_name) {
    try {
        size_t idx = 0;
        int    val = std::stoi(value, &idx);
        if (idx != value.size()) {
            throw std::runtime_error("Invalid integer format for '" + field_name + "': " + value);
        }
        return val;
    } catch (const std::invalid_argument &) {
        throw std::runtime_error("Invalid integer format for '" + field_name + "': " + value);
    } catch (const std::out_of_range &) {
        throw std::runtime_error("Integer value out of range for '" + field_name + "': " + value);
    }
}

/**
 * @brief Validate that a string represents a boolean ("true" or "false").
 *
 * @param value The string to parse.
 * @param field_name The name of the config field (used in error messages).
 * @return The parsed boolean value.
 * @throws std::runtime_error if the string is not "true" or "false".
 */
bool validate_bool(const std::string &value, const std::string &field_name) {
    const std::string lower_value = utils::to_lower_copy(value);

    if (lower_value == "true") return true;
    if (lower_value == "false") return false;

    throw std::runtime_error("Invalid boolean value for '" + field_name +
                             "': expected 'true' or 'false', got '" + value + "'");
}

/**
 * @brief Validate that a string represents a valid octal number (e.g., file mode).
 *
 * @param value The octal string (e.g., "755").
 * @param field_name The name of the config field (used in error messages).
 * @return The parsed mode_t value.
 * @throws std::runtime_error if the format is invalid or out of range.
 */
mode_t validate_octal(const std::string &value, const std::string &field_name) {
    size_t        idx   = 0;
    unsigned long octal = 0;

    try {
        octal = std::stoul(value, &idx, 8);
        if (idx != value.size()) {
            throw std::runtime_error("Invalid characters in octal value for '" + field_name +
                                     "': " + value);
        }
        if (octal > 0777) {
            throw std::runtime_error("Octal value out of range for '" + field_name + "': " + value);
        }
    } catch (const std::invalid_argument &) {
        throw std::runtime_error("Invalid octal format for '" + field_name + "': " + value);
    } catch (const std::out_of_range &) {
        throw std::runtime_error("Octal value out of range for '" + field_name + "': " + value);
    }

    return static_cast<mode_t>(octal);
}

/**
 * @brief Validate and resolve a chown directive (user[:group]).
 *
 * @param user_part The user name.
 * @param group_part The group name (can be empty to use user's default group).
 * @return A pair containing the UID and GID.
 * @throws std::runtime_error if the user or group does not exist.
 */
std::pair<uid_t, gid_t> validate_chown(const std::string &user_part,
                                       const std::string &group_part) {
    uid_t uid = static_cast<uid_t>(-1);  // -1 means "don't change" in chown
    gid_t gid = static_cast<gid_t>(-1);

    if (!user_part.empty()) {
        struct passwd *pw = getpwnam(user_part.c_str());
        if (!pw) throw std::runtime_error("Invalid user in chown: " + user_part);
        uid = pw->pw_uid;

        // If group is empty, use user's default group
        if (group_part.empty()) gid = pw->pw_gid;
    }

    if (!group_part.empty()) {
        struct group *gr = getgrnam(group_part.c_str());
        if (!gr) throw std::runtime_error("Invalid group in chown: " + group_part);
        gid = gr->gr_gid;
    }

    return {uid, gid};
}

/**
 * @brief Validate the 'minfds' directive and raise RLIMIT_NOFILE if needed.
 *
 * @param value The minimum number of file descriptors required.
 * @throws std::runtime_error if the value is negative or limits cannot be set.
 */
void validate_minfds(int value) {
    if (value < 0) throw std::runtime_error("The 'minfds' field must be non-negative.");
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) != 0)
        throw std::runtime_error("Failed to get RLIMIT_NOFILE limit.");

    if ((rlim_t)value > limit.rlim_cur) {
        limit.rlim_cur = value;
        if (setrlimit(RLIMIT_NOFILE, &limit) != 0)
            throw std::runtime_error("Failed to raise RLIMIT_NOFILE limit to required minfds.");
    }
    if (value < 256) Logger::warn("'minfds' value is low; recommended to use at least 1024.");
}

/**
 * @brief Validate the 'minprocs' directive and raise RLIMIT_NPROC if needed.
 *
 * @param value The minimum number of processes required.
 * @throws std::runtime_error if the value is negative or limits cannot be set.
 */
void validate_minprocs(int value) {
    if (value < 0) throw std::runtime_error("The 'minprocs' field must be non-negative.");
    struct rlimit limit;
    if (getrlimit(RLIMIT_NPROC, &limit) != 0)
        throw std::runtime_error("Failed to get RLIMIT_NPROC limit.");

    if ((rlim_t)value > limit.rlim_cur) {
        limit.rlim_cur = value;
        if (setrlimit(RLIMIT_NPROC, &limit) != 0)
            throw std::runtime_error("Failed to raise RLIMIT_NPROC limit to required minprocs.");
    }
    if (value < 50) Logger::warn("'minprocs' value is low; recommended to use at least 200.");
}

/**
 * @brief Validate that a given user exists on the system.
 *
 * @param username The user name to validate.
 * @param field_name The name of the config field (used in error messages).
 * @return The UID of the user.
 * @throws std::runtime_error if the user does not exist.
 */
uid_t validate_user(const std::string &username, const std::string &field_name) {
    struct passwd *pw = getpwnam(username.c_str());

    if (!pw) throw std::runtime_error("Invalid user in '" + field_name + "': " + username);

    return pw->pw_uid;
}

/**
 * @brief Validate that a given group exists on the system.
 *
 * @param groupname The group name to validate.
 * @param field_name The name of the config field (used in error messages).
 * @return The GID of the group.
 * @throws std::runtime_error if the group does not exist.
 */
gid_t validate_group(const std::string &groupname, const std::string &field_name) {
    struct group *gr = getgrnam(groupname.c_str());

    if (!gr) throw std::runtime_error("Invalid group in '" + field_name + "': " + groupname);

    return gr->gr_gid;
}

/**
 * @brief Validate that the 'user' directive is used correctly and refers to a valid user.
 *
 * @param username The user name to validate.
 * @param field_name The name of the config field (used in error messages).
 * @return The original username if valid.
 * @throws std::runtime_error if not run as root or user is invalid.
 */
std::string validate_user_field(const std::string &username, const std::string &field_name) {
    if (geteuid() != 0)
        throw std::runtime_error("The 'user' directive may only be used when running as root.");
    validate_user(username, field_name);

    return username;
}

}  // namespace config_validator
