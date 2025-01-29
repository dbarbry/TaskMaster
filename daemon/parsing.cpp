#include <iostream>
#include <fstream>
#include <vector>
#include "./incs/parsing.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem> 


std::map<std::string, ProgramConfig> parse_config(const std::string& filepath) {
    std::map<std::string, ProgramConfig> programs;
    std::ifstream file(filepath);
    std::string line;
    std::string current_section;

    if (!file.is_open()) {
        std::cerr << "Failed to open config file." << std::endl;
        return programs;
    }

    ProgramConfig current_config;

    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line[0] == '[') {
            if (!current_section.empty()) {
                programs[current_section] = current_config;
            }

            size_t end_pos = line.find(']');
            if (end_pos == std::string::npos) {
                std::cerr << "Invalid section syntax: " << line << std::endl;
                continue; 
            }
            current_section = line.substr(1, end_pos - 1);
            current_config = ProgramConfig();  
        }
        else {
            size_t delimiter_pos = line.find('=');
            if (delimiter_pos == std::string::npos) {
                std::cerr << "Invalid key=value syntax: " << line << std::endl;
                continue;
            }

            std::string key = line.substr(0, delimiter_pos);
            std::string value = line.substr(delimiter_pos + 1);

            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (key == "cmd") {
                current_config.setCmd(value);
            } else if (key == "numprocs") {
                try {
                    current_config.setNumprocs(std::stoi(value));
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid number for numprocs: " << value << std::endl;
                }
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
                std::string temp;
                std::vector<int> exitcodes;
                while (std::getline(ss, temp, ' ')) {
                    try {
                        exitcodes.push_back(std::stoi(temp));
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Invalid exit code: " << temp << std::endl;
                    }
                }
                current_config.setExitcodes(exitcodes);
            } else if (key == "startretries") {
                try {
                    current_config.setStartretries(std::stoi(value));
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid number for startretries: " << value << std::endl;
                }
            } else if (key == "starttime") {
                try {
                    current_config.setStarttime(std::stoi(value));
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid number for starttime: " << value << std::endl;
                }
            } else if (key == "stopsignal") {
                current_config.setStopsignal(value);
            } else if (key == "stoptime") {
                try {
                    current_config.setStoptime(std::stoi(value));
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid number for stoptime: " << value << std::endl;
                }
            } else if (key == "stdout") {
                current_config.setStdoutFile(value);
            } else if (key == "stderr") {
                current_config.setStderrFile(value);
            } else if (key == "env") {
                std::stringstream ss(value);
                std::string env_pair;
                std::map<std::string, std::string> env_map;
                while (std::getline(ss, env_pair, ' ')) {
                    size_t equal_pos = env_pair.find('=');
                    if (equal_pos != std::string::npos) {
                        std::string env_key = env_pair.substr(0, equal_pos);
                        std::string env_value = env_pair.substr(equal_pos + 1);
                        env_map[env_key] = env_value;
                    }
                }
                current_config.setEnv(env_map);
            }
        }
    }
    if (!current_section.empty()) {
        programs[current_section] = current_config;
    }

    return programs;
}

void log_config(const std::map<std::string, ProgramConfig>& programs) {
    for (const auto& program : programs) {
        std::cout << "Program: " << program.first << std::endl;
        program.second.logConfig();
        std::cout << std::endl;
    }
}

void parsing(char **args)
{
    if (args[1] == nullptr) {
        std::cout << "Aucun fichier spécifié." << std::endl;
        return;
    }
    std::string filename = args[1];
    if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".conf") {
        std::cout << "Le fichier doit avoir une extension .conf." << std::endl;
        return;
    }
    if (!std::filesystem::exists(filename)) {
        std::cout << "Le fichier spécifié n'existe pas : " << filename << std::endl;
        return;
    }
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Impossible d'ouvrir le fichier : " << filename << std::endl;
        return;
    }
    std::map<std::string, ProgramConfig> programs = parse_config(filename); 

    for (auto it = programs.begin(); it != programs.end(); ) {
    if (!it->second.isValid()) {
        std::cerr << "Erreur: Configuration invalide pour le programme " << it->first << ". Il sera ignoré." << std::endl;
        it = programs.erase(it);
    } else {
        ++it;
    }
}

if (programs.empty()) {
    std::cerr << "Aucun programme valide dans la configuration. Arrêt du programme." << std::endl;
    exit(EXIT_FAILURE);
}

    log_config(programs);   
}
