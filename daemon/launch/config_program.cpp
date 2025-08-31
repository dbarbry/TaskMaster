#include "./config_program.hpp"

#include "./logger.hpp"
#include "parsing.hpp"

/**
 * @brief Parse un fichier de configuration TaskMaster
 * @param filepath Chemin du fichier de configuration
 * @return Une map des programmes et leurs configurations
 */
std::map<std::string, ProgramConfig> parse_config(const std::string& filepath) {
    std::map<std::string, ProgramConfig> programs;
    std::ifstream                        file(filepath);
    std::string                          line;
    std::string                          current_program_name;
    ProgramConfig                        current_program;

    if (!file.is_open()) {
        Logger::error("Failed to open config file: " + filepath);
        return programs;
    }

    while (std::getline(file, line)) {
        line = utils::trim(line);
        if (utils::is_comment_or_empty(line)) continue;

        if (line.front() == '[' && line.back() == ']') {
            current_program_name = line.substr(1, line.size() - 2);
            current_program_name = utils::to_lower_copy(current_program_name);
            current_program      = ProgramConfig();
            continue;
        } else if (line.front() == '[' && line.back() != ']' ||
                   line.front() != '[' && line.back() == ']') {
            Logger::error("Invalid program syntax: " + line);
            continue;
        } else {
            auto kv_pair = config_parser::parse_line(line);
            if (!kv_pair || current_program_name.empty()) continue;

            const std::string& key   = utils::to_lower_copy(kv_pair->first);
            const std::string& value = kv_pair->second;

            try {
                // Configuration options
                if (key == "cmd") {
                    current_program.setCommand(config_parser::parse_command(value));
                } else if (key == "numprocs") {
                    current_program.setNumprocs(config_validator::validate_integer(value, key));
                } else if (key == "umask") {
                    current_program.setUmask(config_validator::validate_octal(value, key));
                } else if (key == "workingdir") {
                    current_program.setWorkingDir(config_validator::validate_path(value, key));
                } else if (key == "autostart") {
                    current_program.setAutostart(config_validator::validate_bool(value, key));
                } else if (key == "autorestart") {
                    current_program.setAutorestart(string_to_autorestart(value));
                } else if (key == "exitcodes") {
                    std::vector<int>  exitcodes;
                    std::stringstream ss(value);
                    std::string       token;
                    while (std::getline(ss, token, ',')) {
                        exitcodes.push_back(
                            config_validator::validate_integer(utils::trim(token), key));
                    }
                    current_program.setExitcodes(exitcodes);
                } else if (key == "startretries") {
                    current_program.setStartretries(config_validator::validate_integer(value, key));
                } else if (key == "starttime" || key == "startsecs") {  // Support both names
                    current_program.setStartsecs(config_validator::validate_integer(value, key));
                } else if (key == "stopsignal") {
                    std::string upper_value = utils::to_upper_copy(value);
                    current_program.setStopsignal(string_to_stopsignal(upper_value));
                } else if (key == "stoptime" || key == "stopwaitsecs") {  // Support both names
                    current_program.setStopwaitsecs(config_validator::validate_integer(value, key));
                } else if (key == "stdout" || key == "stdout_logfile") {  // Support both names
                    current_program.setStdoutLogfile(config_validator::validate_path(value, key));
                } else if (key == "stderr" || key == "stderr_logfile") {  // Support both names
                    current_program.setStderrLogfile(config_validator::validate_path(value, key));
                } else if (key == "env" || key == "environment") {  // Support both names
                    current_program.setEnvironment(config_parser::parse_environment(value));
                } else {
                    Logger::error("Unknown configuration key: " + key);
                }
            } catch (const std::invalid_argument& e) {
                Logger::error(e.what());
            } catch (const std::exception& e) {
                Logger::error(e.what());
            }
        }
    }

    // Save the last section if exists
    if (!current_program_name.empty()) {
        programs[current_program_name] = current_program;
    }

    return programs;
}

/**
 * @brief Affiche la configuration des programmes
 * @param programs Map des programmes et leurs configurations
 */
void log_config(const std::map<std::string, ProgramConfig>& programs) {
    for (const auto& [name, config] : programs) config.logConfig(name);
}

/**
 * @brief Analyse un fichier de configuration et valide les programmes
 * @param filename Chemin du fichier de configuration
 * @return Map des programmes valides et leurs configurations
 */
std::map<std::string, ProgramConfig> parsing(std::string filename) {
    std::map<std::string, ProgramConfig> programs = parse_config(filename);

    // Filtrer les configurations invalides
    for (auto it = programs.begin(); it != programs.end();) {
        if (!it->second.isValid(it->first)) {
            Logger::error("Invalid configuration for service: " + it->first + ". Service skipped.");
            it = programs.erase(it);
        } else {
            ++it;
        }
    }

    if (programs.empty()) Logger::error("No programs in memory yet.");

    return programs;
}
