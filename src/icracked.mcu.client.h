//
// Created by Kousha Talebian on 4/7/15.
//

#ifndef MCU_ICRACKED_MCU_CLIENT_H
#define MCU_ICRACKED_MCU_CLIENT_H

    #define SOCKET_CONNECT_TIMEOUT_SEC 1
    #define SOCKET_CONNECT_TIME_USEC 0

    // Functions
    void client (char *buffer, int buffer_size, int portno, char * cmd);
    void error (const char *);
    void init_socket(int);
    int timeout_serial_read(int);

#endif //MCU_ICRACKED_MCU_CLIENT_H
