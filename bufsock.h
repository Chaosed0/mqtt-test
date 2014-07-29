
#ifndef BUFSOCK_H
#define BUFSOCK_H

#define BUF_SIZE 1024

typedef struct bufsock {
    int sockfd;
    size_t buf_pos;
    size_t buf_end;
    char buffer[BUF_SIZE];
} bufsock;

bufsock *bufsock_init(bufsock *sock, char *hostname, char *port);
ssize_t buf_send(bufsock *sock, char *buf, size_t len, int flags);
ssize_t buf_recv(bufsock *sock, char *dest, size_t amt, int flags);
void bufsock_close(bufsock *sock);

#endif
