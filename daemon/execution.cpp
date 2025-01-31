#include "./incs/execution.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

void exec_programs(const std::map<std::string, ProgramConfig> &programs) {
    std::vector<pid_t> pids;

    for (const auto &[name, config] : programs) {
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "Erreur: Impossible de forker pour " << name << std::endl;
            continue;
        }

        if (pid == 0) {
            std::cout << "Lancement du programme: " << name << " (" << config.getCmd() << ")"
                      << std::endl;

            std::vector<std::string> args;
            std::string              cmd = config.getCmd();

            std::istringstream iss(cmd);
            std::string        arg;
            while (iss >> arg) {
                args.push_back(arg);
            }

            if (args.empty()) {
                std::cerr << "Erreur: Commande vide pour " << name << std::endl;
                _exit(EXIT_FAILURE);
            }

            std::vector<char *> argv;
            for (auto &s : args) argv.push_back(s.data());
            argv.push_back(nullptr);

            std::cout << "Arguments à exécuter : " << std::endl;
            for (size_t i = 0; i < args.size(); ++i) {
                std::cout << "[" << i << "] " << args[i] << std::endl;
            }

            std::cout << "[PID " << getpid() << "] Exécution de: " << argv[0] << std::endl;

            execvp(argv[0], argv.data());

            std::cerr << "Erreur: Échec de l'exécution de " << config.getCmd() << std::endl;
            _exit(EXIT_FAILURE);
        } else {
            pids.push_back(pid);
        }
    }

    for (pid_t pid : pids) {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            std::cout << "[PID " << pid << "] Terminé avec le code: " << WEXITSTATUS(status)
                      << std::endl;
        } else if (WIFSIGNALED(status)) {
            std::cout << "[PID " << pid << "] Tué par le signal: " << WTERMSIG(status) << std::endl;
        }
    }
}
