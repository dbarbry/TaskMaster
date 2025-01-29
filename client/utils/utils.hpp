#ifndef UTILS_HPP
#define UTILS_HPP

void    setup_signal_handlers(void);

int     open_pty(void);
int     open_slave(int master_fd);

void    attach_terminal(int slave_fd);
void    write_to_pty(int master_fd);
void    read_from_pty(int fd);

#endif
