#ifndef PARSING_HPP
#define PARSING_HPP

#include <glob.h>
#include <grp.h>
#include <pwd.h>
#include <sys/resource.h>
#include <unistd.h>

#include <filesystem>
#include <optional>
#include <regex>
#include <set>

#include "../logger.hpp"

namespace utils {
std::string              trim(const std::string &line);
std::vector<std::string> expand_glob(const std::string &pattern);
std::string              to_lower_copy(const std::string &input);
std::string              to_upper_copy(const std::string &input);
bool                     is_comment_or_empty(const std::string &line);
}  // namespace utils

namespace config_parser {
using KeyValue = std::pair<std::string, std::string>;
std::optional<KeyValue>             parse_line(const std::string &raw_line);
std::pair<std::string, std::string> parse_chown(const std::string &chown_str);
std::map<std::string, std::string>  parse_environment(const std::string &line);
std::string                         parse_command(const std::string &line);
}  // namespace config_parser

namespace config_validator {
std::filesystem::path validate_path(const std::string &value, const std::string &field_name);
std::filesystem::path validate_folder(const std::filesystem::path &path,
                                      const std::string           &field_name);
std::filesystem::path validate_dir(const std::string &value, const std::string &field_name);
std::filesystem::path validate_file_exists(const std::string &value, const std::string &field_name);
int                   validate_integer(const std::string &value, const std::string &field_name);
bool                  validate_bool(const std::string &value, const std::string &field_name);
mode_t                validate_octal(const std::string &value, const std::string &field_name);
std::pair<uid_t, gid_t> validate_chown(const std::string &user_part, const std::string &group_part);
void                    validate_minfds(int value);
void                    validate_minprocs(int value);
uid_t                   validate_user(const std::string &username, const std::string &field_name);
gid_t                   validate_group(const std::string &groupname, const std::string &field_name);
std::string validate_user_field(const std::string &username, const std::string &field_name);
}  // namespace config_validator

#endif  // PARSING_HPP
