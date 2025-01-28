#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <string>
#include <vector>

#define SOCKET_PATH "/tmp/taskmaster_socket"
#define BUFFER_SIZE 1024

class Shell {
    public:
        void    run(int fd) {
            char    *input;

            while (true) {
                input = readline("taskmaster> ");
                std::string cmd(input);

                if(!cmd.empty()) {
                    add_history(input);
                    std::string clean_cmd = parse_cmd(cmd);

                    send_cmd(fd, clean_cmd);
                }
                free(input);
            }
        }
    
    private:
        std::string parse_cmd(const std::string &cmd) {
            std::string clean_cmd = cmd;

            clean_cmd.erase(0, clean_cmd.find_first_not_of(" "));

            return clean_cmd;
        }

        void    send_cmd(int fd, const std::string &cmd) {
            char    buffer[BUFFER_SIZE] = {0};
            
            write(fd, cmd.c_str(), cmd.size());
            read(fd, buffer, BUFFER_SIZE);

            std::cout << "Server: " << buffer << std::endl;
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

    shell.run(fd);

    close(fd);
    return 0;
}
