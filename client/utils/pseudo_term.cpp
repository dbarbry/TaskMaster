#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <cstdio>

int open_pty(void) {
    int master_fd = posix_openpt(O_RDWR | O_NOCTTY);

    if (master_fd == -1) {
        perror("posix_openpt failed");
        return -1;
    }
    // to make sure slave can be accessed
    if (grantpt(master_fd) == -1 || unlockpt(master_fd) == -1) {
        perror("grantpt/unlockpt failed");
        close(master_fd);
        return -1;
    }

    return master_fd;
}

int open_slave(int master_fd) {
    char    *name = ptsname(master_fd);
    int     slave_fd;

    if (!name) {
        perror("ptsname failed");
        return -1;
    }
    
    slave_fd = open(name, O_RDWR);
    if (slave_fd == -1) {
        perror("open slave failed");
        return -1;
    }

    return slave_fd;
}

void    attach_terminal(int slave_fd) {
    setsid();
    ioctl(slave_fd, TIOCSCTTY, 0);

    dup2(slave_fd, STDIN_FILENO);
    dup2(slave_fd, STDOUT_FILENO);
    dup2(slave_fd, STDERR_FILENO);

    close(slave_fd);
}

void    write_to_pty(int master_fd) {
    std::string msg = "Hey there\n";
    write(master_fd, msg.c_str(), msg.length());
}

void    read_from_pty(int fd) {
    char    buffer[256];
    ssize_t len = read(fd, buffer, sizeof(buffer) - 1);

    if (len > 0) {
        buffer[len] = '\0';
        std::cout << "Received: " << buffer;
    } else {
        perror("read failed");
    }
}