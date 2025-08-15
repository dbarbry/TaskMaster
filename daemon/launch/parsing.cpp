#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "./config_program.hpp"
#include "./logger.hpp"

/**
 * @brief Parse une ligne de configuration au format "clé=valeur"
 * @param line La ligne à parser
 * @return Une paire contenant la clé et la valeur, ou nullopt si la ligne est invalide
 */
std::optional<std::pair<std::string, std::string>> parse_config_line(const std::string& line) {
    size_t delimiter_pos = line.find('=');
    if (delimiter_pos == std::string::npos) {
        Logger::error("Invalid key=value syntax: " + line);
        return std::nullopt;
    }

    std::string key   = line.substr(0, delimiter_pos);
    std::string value = line.substr(delimiter_pos + 1);

    // Trim whitespace
    auto trim = [](std::string& s) {
        s.erase(0, s.find_first_not_of(" \t"));
        s.erase(s.find_last_not_of(" \t") + 1);
    };
    trim(key);
    trim(value);

    return std::make_pair(key, value);
}

/**
 * @brief Parse les variables d'environnement du format "KEY=VALUE KEY2=VALUE2"
 * @param value La chaîne contenant les paires clé-valeur
 * @return Une map des variables d'environnement
 */
std::map<std::string, std::string> parse_environment(const std::string& value) {
    std::stringstream                  ss(value);
    std::string                        env_pair;
    std::map<std::string, std::string> env_map;

    while (std::getline(ss, env_pair, ' ')) {
        size_t equal_pos = env_pair.find('=');
        if (equal_pos != std::string::npos) {
            std::string env_key   = env_pair.substr(0, equal_pos);
            std::string env_value = env_pair.substr(equal_pos + 1);

            // Enlever les guillemets si présents
            if (!env_value.empty() && env_value.front() == '"' && env_value.back() == '"') {
                env_value = env_value.substr(1, env_value.length() - 2);
            }

            env_map[env_key] = env_value;
        }
    }
    return env_map;
}

/**
 * @brief Parse un fichier de configuration TaskMaster
 * @param filepath Chemin du fichier de configuration
 * @return Une map des programmes et leurs configurations
 */
std::map<std::string, ProgramConfig> parse_config(const std::string& filepath) {
    std::map<std::string, ProgramConfig> programs;
    std::ifstream                        file(filepath);
    std::string                          line;
    std::string                          current_section;

    if (!file.is_open()) {
        Logger::error("Failed to open config file: " + filepath);
        return programs;
    }

    ProgramConfig current_config;

    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Ignore empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Handle section headers [section_name]
        if (line[0] == '[') {
            // Save previous section if exists
            if (!current_section.empty()) {
                programs[current_section] = current_config;
            }

            size_t end_pos = line.find(']');
            if (end_pos == std::string::npos) {
                Logger::error("Invalid section syntax: " + line);
                continue;
            }
            current_section = line.substr(1, end_pos - 1);
            current_config  = ProgramConfig();
        }
        // Handle key=value pairs
        else {
            auto kv_pair = parse_config_line(line);
            if (!kv_pair) {
                continue;
            }

            const std::string& key   = kv_pair->first;
            const std::string& value = kv_pair->second;

            try {
                // Configuration options
                if (key == "cmd") {
                    current_config.setCommand(value);
                } else if (key == "numprocs") {
                    current_config.setNumprocs(std::stoi(value));
                } else if (key == "umask") {
                    current_config.setUmask(value);
                } else if (key == "workingdir") {
                    current_config.setWorkingDir(value);
                } else if (key == "autostart") {
                    current_config.setAutostart(value == "true");
                } else if (key == "autorestart") {
                    current_config.setAutorestart(value);
                } else if (key == "exitcodes") {
                    std::stringstream ss(value);
                    std::string       temp;
                    std::vector<int>  exitcodes;

                    while (std::getline(ss, temp, ' ')) {
                        exitcodes.push_back(std::stoi(temp));
                    }
                    current_config.setExitcodes(exitcodes);
                } else if (key == "startretries") {
                    current_config.setStartretries(std::stoi(value));
                } else if (key == "starttime" || key == "startsecs") {  // Support both names
                    current_config.setStartsecs(std::stoi(value));
                } else if (key == "stopsignal") {
                    current_config.setStopsignal(value);
                } else if (key == "stoptime" || key == "stopwaitsecs") {  // Support both names
                    current_config.setStopwaitsecs(std::stoi(value));
                } else if (key == "stdout" || key == "stdout_logfile") {  // Support both names
                    current_config.setStdoutLogfile(value);
                } else if (key == "stderr" || key == "stderr_logfile") {  // Support both names
                    current_config.setStderrLogfile(value);
                } else if (key == "env" || key == "environment") {  // Support both names
                    current_config.setEnvironment(parse_environment(value));
                } else {
                    Logger::error("Unknown configuration key: " + key);
                }
            } catch (const std::invalid_argument& e) {
                Logger::error("Invalid value for " + key + ": " + value + " (" + e.what() + ")");
            } catch (const std::exception& e) {
                Logger::error("Error processing key " + key + ": " + e.what());
            }
        }
    }

    // Save the last section if exists
    if (!current_section.empty()) {
        programs[current_section] = current_config;
    }

    return programs;
}

/**
 * @brief Affiche la configuration des programmes
 * @param programs Map des programmes et leurs configurations
 */
void log_config(const std::map<std::string, ProgramConfig>& programs) {
    for (const auto& [name, config] : programs) {
        Logger::info("Program: " + name);
        config.logConfig();
    }
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
        if (!it->second.isValid()) {
            Logger::error("Invalid configuration for service: " + it->first + ". Service skipped.");
            it = programs.erase(it);
        } else {
            ++it;
        }
    }

    // Vérifier qu'il reste au moins un programme valide
    if (programs.empty()) {
        Logger::error("No valid service configuration found. Exiting...");
        exit(1);
    }

    return programs;
}
