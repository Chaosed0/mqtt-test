#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "bufsock.h"

bufsock *bufsock_init(bufsock *sock, char *hostname, char *port) {
    struct addrinfo *result, *rp;
    struct addrinfo hints;
    int sockfd;
    int rc;

    /* XXX: This should probably be parametrized */
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if((rc = getaddrinfo(hostname, port, &hints, &result)) != 0) {
        fprintf(stderr, "getaddrinfo: Could not get addrinfo of host %s:%s: %s!\n",
                hostname, port, gai_strerror(rc));
        return NULL;
    }

    for(rp = result; rp != NULL; rp = rp->ai_next) {
        if((sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
            continue;
        }

        if(connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }

        close(sockfd);
    }

    if(rp == NULL) {
        fprintf(stderr, "Could not connect socket\n");
        return NULL;
    }
    
    sock->sockfd = sockfd;
    sock->buf_pos = 0;
    sock->buf_end = 0;
    return sock;
}

static ssize_t check_read(bufsock *sock, int flags) {
    ssize_t bytes_read;
    if(sock->buf_pos >= sock->buf_end) {
        if((bytes_read = recv(sock->sockfd, sock->buffer, BUF_SIZE, flags)) < 0) {
            fprintf(stderr, "mqtt_buffer: could not fill buffer\n");
        }
        sock->buf_pos = 0;
        sock->buf_end = (ssize_t)bytes_read;
    }
    return sock->buf_end;
}

ssize_t buf_send(bufsock *sock, char *buf, size_t len, int flags) {
    return send(sock->sockfd, buf, len, flags);
}

ssize_t buf_recv(bufsock *sock, char *buf, size_t len, int flags) {
    size_t amt_copied = 0;
    ssize_t amt_read;
    while(amt_copied < len) {
        /* Figure out if we have enough in the socket's buffer to fulfill
         * the request */
        int copy_size = (sock->buf_end - sock->buf_pos < len - amt_copied ?
                sock->buf_end - sock->buf_pos :
                len - amt_copied);
        if((amt_read = check_read(sock, flags)) < 0) {
            /* Error occured */
            return amt_read;
        }
        /* Copy what we have to the return buffer */
        memcpy(buf + amt_copied, sock->buffer + sock->buf_pos, copy_size);
        amt_copied += copy_size;
        sock->buf_pos += copy_size;
    }
    return amt_copied;
}

void bufsock_close(bufsock *sock) {
    close(sock->sockfd);
}
