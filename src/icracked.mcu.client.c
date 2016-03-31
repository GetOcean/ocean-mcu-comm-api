#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "icracked.mcu.util.h"
#include "icracked.mcu.client.h"

int sockfd;
struct sockaddr_in serv_addr;
struct hostent *server;

void client(char *bufferRead, int buffer_size, int portno, char * cmd) {
    // Initialize the logger file
    openlog("mcu.log", LOG_CONS | LOG_PERROR, LOG_USER);

    // Prepare socket
    init_socket(portno);

    // Send command down the socket
    char *str_to_send = addCarriageChar(stripNewLine(cmd));
    int n = write(sockfd, str_to_send, strlen(str_to_send));
    if (n == -1) {
        syslog(LOG_ERR, "error writing to socket");
    }

    // Listen for confirmation
    bzero(bufferRead, buffer_size);
    n = timeout_serial_read(sockfd);
    if (n == -1) {
        syslog(LOG_ERR, "error reading from socket");
    } else if (n == 0) {
        //syslog(LOG_ERR, "timeout to read response from cmd");
    } else {
        n = read(sockfd, bufferRead, buffer_size);
        if (n == -1) {
            syslog(LOG_ERR, "error reading from socket");
        }
    }

    // free memory
    free(str_to_send);

    // Close socket and quit
    close(sockfd);
}

void init_socket(int portno) {
    long arg;
    fd_set fdset;
    struct timeval tv;
    int valopt;
    socklen_t lon;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        syslog(LOG_ERR, "error opening socket");
        exit(1);
    }

    // Find device hostname
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    server = gethostbyname(hostname);
    if (server == NULL) {
        syslog(LOG_ERR, "no such hostname: %s", server);
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    // Set non-blocking while testing connection
    arg = fcntl(sockfd, F_GETFL, NULL);
    if(arg < 0) {
        syslog(LOG_ERR, "Error fcntl(..., F_GETFL) (%s)", strerror(errno));
        exit(1);
    }
    arg |= O_NONBLOCK;
    if( fcntl(sockfd, F_SETFL, arg) < 0) {
        syslog(LOG_ERR, "Error fcntl(..., F_SETFL) (%s)", strerror(errno));
        exit(1);
    }

    // Attempt to connect
    int res = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (res < 0) {
        if (errno == EINPROGRESS) {
            do {
                tv.tv_sec = SOCKET_CONNECT_TIMEOUT_SEC;
                tv.tv_usec = SOCKET_CONNECT_TIME_USEC;
                FD_ZERO(&fdset);
                FD_SET(sockfd, &fdset);
                res = select(sockfd+1, NULL, &fdset, NULL, &tv);

                if (res < 0 && errno != EINTR) {
                    syslog(LOG_ERR, "Error connecting %d - %s", errno, strerror(errno));
                    exit(1);
                } else if (res > 0) {
                    // Socket selected for write
                    lon = sizeof(int);
                    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) {
                        syslog(LOG_ERR, "Error in getsockopt() %d - %s", errno, strerror(errno));
                        exit(1);
                    }
                    // Check the value returned...
                    if (valopt) {
                        syslog(LOG_ERR, "Error in delayed connection() %d - %s", valopt, strerror(valopt));
                        exit(1);
                    }
                    break;
                } else {
                    syslog(LOG_ERR, "socket connection timedout; is interface lo up?");
                    exit(1);
                }
            } while (1);

        } else {
            syslog(LOG_ERR, "Error connecting %d - %s", errno, strerror(errno));
            exit(1);
        }
    }

    // Set to blocking mode
    arg = fcntl(sockfd, F_GETFL, NULL);
    if(arg < 0) {
        syslog(LOG_ERR, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno));
        exit(1);
    }
    arg &= (~O_NONBLOCK);
    if(fcntl(sockfd, F_SETFL, arg) < 0) {
        syslog(LOG_ERR, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno));
        exit(1);
    }
}

int timeout_serial_read(int fd) {
    fd_set set;
    struct timeval timeout;

    FD_ZERO(&set);
    FD_SET(fd, &set);
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000 * 10;

    return select(fd + 1, &set, NULL, NULL, &timeout);
}
