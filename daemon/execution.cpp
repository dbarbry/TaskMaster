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

    for (const auto &[name, config] : programs) {
        const int max_retries = config.getStartretries();
        int retries = 0;

        while (retries < max_retries) {
            pid_t pid = fork();

            if (pid < 0) {
                std::cerr << "Fork failed for: " << name << std::endl;
                break;
            }

            if (pid == 0) {
                std::cout << "Launching: " << name << " (" << config.getCmd() << ")" << std::endl;

                std::vector<std::string> args;
                std::string              cmd = config.getCmd();

                std::istringstream iss(cmd);
                std::string        arg;
                while (iss >> arg) {
                    args.push_back(arg);
                }

                if (args.empty()) {
                    std::cerr << "Empty command for: " << name << std::endl;
                    _exit(EXIT_FAILURE);
                }

                std::vector<char *> argv;
                for (auto &s : args) argv.push_back(s.data());
                argv.push_back(nullptr);

                std::cout << "[PID " << getpid() << "] Execution of: " << argv[0] << std::endl;

                execvp(argv[0], argv.data());

                std::cerr << "Execution failed for: " << config.getCmd() << std::endl;
                _exit(EXIT_FAILURE);
            } else { 
                int status;
                waitpid(pid, &status, 0);

                if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            std::cout << "[PID " << pid << "] ended with code: " << exit_code << std::endl;

            const std::vector<int>& valid_exit_codes = config.getExitcodes();

            if (std::find(valid_exit_codes.begin(), valid_exit_codes.end(), exit_code) != valid_exit_codes.end()) {
                std::cout << "[PID " << pid << "] Exit code is allowed, no restart needed." << std::endl;
                break;
            }

            std::cout << "[PID " << pid << "] Unexpected exit code, restarting..." << std::endl;
            } 
            else if (WIFSIGNALED(status)) {
            std::cout << "[PID " << pid << "] killed by signal: " << WTERMSIG(status) << std::endl;
            }

                retries++;
                if (retries < max_retries) {
                    std::cerr << "Restarting " << name << " (" << retries << "/" << max_retries << ")" << std::endl;
                } else {
                    std::cerr << "Max retries reached for " << name << ", giving up." << std::endl;
                }
            }
        }
    }
}
