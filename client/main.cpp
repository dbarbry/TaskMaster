#include "main.hpp"

#define SOCKET_PATH "/tmp/taskmaster_socket"
#define BUFFER_SIZE 1024

class Shell {
    public:
        void    run(int fd) {
            char    *input;

            while (true) {
                input = readline("\033[34mtaskmaster\033[0m$ ");
                if (!input) { // ctrl D
                    std::cout << "Leaving..." << std::endl;
                    break;
                }

                std::string cmd(input);
                free(input);

                if(!cmd.empty()) {
                    add_history(cmd.c_str());
                    std::string clean_cmd = parse_cmd(cmd);

                    if (!analyze_cmd(fd, clean_cmd)) {
                        break;
                    }
                }
            }
        }
    
    private:
        std::string parse_cmd(const std::string &cmd) {
            std::string clean_cmd = cmd;

            clean_cmd.erase(0, clean_cmd.find_first_not_of(" "));

            return clean_cmd;
        }

        bool    send_cmd(int fd, const std::string &cmd) {
            char buffer[BUFFER_SIZE] = {0};
            std::string message = cmd + "\n";
            ssize_t     bytes_read;
            
            if (write(fd, message.c_str(), message.size()) <= 0) {
                perror("write failed");
                return false;
            }

            bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
            if (bytes_read < 0) {
                perror("read failed");
                return false;
            } else if (bytes_read == 0) {
                std::cerr << "Server closed the connection.\n";
                return false;
            }

            std::cout << "Server: " << buffer;
            return true;
        }

        bool    analyze_cmd(int fd, const std::string &cmd) {
            if (cmd == "help") {
                std::cout << "Server commands:" << std::endl;
                std::cout << "  halt <serviceName> - pause a service" << std::endl;
                std::cout << "  stop <serviceName> - stop a service" << std::endl;
                std::cout << "  restart <serviceName> - restart a service" << std::endl;
                std::cout << "  reload <serviceName> - reload a service" << std::endl;
                std::cout << std::endl << "Client commands:" << std::endl;
                std::cout << "  exit - exit the client" << std::endl << std::endl;
                return true;
            } else if (cmd == "exit") {
                std::cout << "Leaving..." << std::endl;
                return false;
            }

            if (!send_cmd(fd, cmd)) {
                std::cout << "Daemon crashed" << std::endl;
                return false;
            }
            return true;
        }
};

int    main(void) {
    int                 fd;
    Shell               shell;
    struct  sockaddr_un address;
    char                buffer[BUFFER_SIZE];

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(1);
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("connect failed");
        exit(1);
    }

    std::cout << "Welcome to taskmaster client." << std::endl;
    std::cout << "type 'help' for help." << std::endl;

    setup_signal_handlers();
    shell.run(fd);

    close(fd);
    return 0;
}
