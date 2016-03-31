#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <syslog.h>
#include <time.h>
#include <math.h>

#include "mcu.h"
#include "icracked.mcu.util.h"
#include "icracked.mcu.serial.h"

int main(int argc, char *argv[]) {
    // Initialize the logger file
    openlog("mcu.serial.log", LOG_CONS | LOG_PERROR, LOG_USER);

    // We need the port to listen to commands writing
    if (argc < 2) {
        syslog(LOG_ERR, "no port was provided");
        exit(1);
    }
    int portno = atoi(argv[1]);

    // Initialize serial port
    init_serial();

    // Initialize server for listening to socket
    init_server(portno);

    // Initialize daemon and run the process in the background
    init_daemon();

    // Initialize global parameters
    clock_gettime(CLOCK_REALTIME, &btnReleasedTime);
    clock_gettime(CLOCK_REALTIME, &btnPressedTime);
    isButtonPressed = false;
    isSystemShuttingDown = false;

    int n;
    while (1) {
        // Start listening to socket for commands
        listen(sockfd,5);
        clilen = sizeof(cli_addr);

        // Wait for command but timeout
        n = timeout_serial_read(sockfd);
        if (n == -1) {
            // Error. Handled below
        } else if (n == 0) {
            // This is for READING button
            read_from_mcu();
        } else {
            // This is for WRITING COMMANDS
            write_to_mcu();
        }

        // List all action functions below
        action_shutdown();
    }

    close(sockfd);
    return 0;
}

void write_to_mcu() {
    char bufferCommand[BUFFER_COMMAND_SIZE];
    char bufferResponse[BUFFER_RESPONSE_SIZE];
    bzero(bufferCommand, BUFFER_COMMAND_SIZE);
    bzero(bufferResponse, BUFFER_RESPONSE_SIZE);

    int sockfd_command = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (sockfd_command < 0) {
        syslog(LOG_ERR, "could not accept socket port");
    }

    // Now read the command
    int n = read(sockfd_command, bufferCommand, BUFFER_COMMAND_SIZE);
    if (n < 0) {
        syslog(LOG_ERR, "could not read command from socket port");
    }

    // Write the command to the serial
    write(fserial, bufferCommand, strlen (bufferCommand));

    // Sleep enough to transmit data
    usleep ((strlen(bufferCommand) + 3) * 100);

    // Now read the response, but timeout if nothing returned
    n = timeout_serial_read(fserial);

    if (n == -1) {
        // Error. Handled below
    } else if (n == 0) {
        //syslog(LOG_WARNING, "timeout, did not receive response from MCU");
    } else {
        n = read(fserial, bufferResponse, sizeof bufferResponse);
        // Error reading from the socket
        if (n < 0) {
            syslog(LOG_WARNING, "could not read response from serial port");
        } else {
            syslog(LOG_INFO, bufferResponse);
        }
    }

    if (n > 0) {
        // Send MCU response to client
        n = write(sockfd_command, bufferResponse, sizeof(bufferResponse));
        if (n < 0) {
            syslog(LOG_ERR, "could not write confirmation to socket port");
        }
    }

    close(sockfd_command);
}

void read_from_mcu() {
    char bufferResponse[BUFFER_RESPONSE_SIZE];
    bzero(bufferResponse, BUFFER_RESPONSE_SIZE);

    // Wait for button press, but timeout
    int n = timeout_serial_read(fserial);

    // Timeout is not important; it means nothing was read

    if (n == -1) {
        syslog(LOG_ERR, "could not read MCU serial port");
    } else if (n > 0) {
        // Read MCU events
        n = read(fserial, bufferResponse, BUFFER_RESPONSE_SIZE);
        if (n > 0) {
            syslog(LOG_INFO, "received response from MCU: %s", bufferResponse);

            // This is a button event
            if (strstr(stripNewLine(bufferResponse), EVENT_BUTTON) != NULL) {
                handle_button_press(bufferResponse);
            }

            // Battery broadcast
            if (strstr(stripNewLine(bufferResponse), EVENT_BATTERY) != NULL) {
                FILE *file = fopen(BATTERY_FILE, "w");
                if (file == NULL) {
                    syslog(LOG_ERR, "error opening %s", BATTERY_FILE);
                } else {
                    char *level;
                    char bufferDuplicate[BUFFER_RESPONSE_SIZE];
                    bzero(bufferDuplicate, BUFFER_RESPONSE_SIZE);
                    strcpy(bufferDuplicate, bufferResponse);
                    strtok_r(bufferDuplicate, ":", &level);
                    fprintf(file, "%s", trim(level));
                }
                fclose(file);
            }

            // Start bypass mode
            if (strstr(stripNewLine(bufferResponse), EVENT_BYPASS_START) != NULL) {
                snprintf(usbConsoleAction, USB_CONSOLE_BUFFER_SIZE, "%s restart", USB_CONSOLE);
                system(usbConsoleAction);
            }
        } else {
            syslog(LOG_WARNING, "could not read button press");
        }
    }
}

// Times button up/down events
void handle_button_press (char *event) {
    // Button down pressed. Count time
    if (strstr(stripNewLine(event), "ev b1") != NULL) {
        isButtonPressed = true;
        clock_gettime(CLOCK_REALTIME, &btnPressedTime);
    } else if (strstr(stripNewLine(event), "ev b2") != NULL) {
        // Button released; let's compute time different
        isButtonPressed = false;
        clock_gettime(CLOCK_REALTIME, &btnReleasedTime);
    } else {
        syslog(LOG_WARNING, "unknown button event type :%s", event);
    }
}

// determines if system should shutdown
void action_shutdown() {
    bool buttonHeldAndReleased = false;
    bool buttonHeldDownForShutdown = false;

    // Button is still pressed
    if (isButtonPressed) {
        struct timespec currentTime;
        clock_gettime(CLOCK_REALTIME, &currentTime);
        long int diffSecond = (currentTime.tv_sec - btnPressedTime.tv_sec);
        long int diffNSecond = (currentTime.tv_nsec - btnPressedTime.tv_nsec);
        long int holdMSecond = diffSecond * 1000 + diffNSecond / 1000000;
        buttonHeldDownForShutdown = holdMSecond >= SHUTDOWN_TIME_HELD_MSEC;
    } else {
        // Button is released
        long int diffSecond = (btnReleasedTime.tv_sec - btnPressedTime.tv_sec);
        long int diffNSecond = (btnReleasedTime.tv_nsec - btnPressedTime.tv_nsec);
        long int holdMSecond = diffSecond * 1000 + diffNSecond / 1000000;
        buttonHeldAndReleased = SHUTDOWN_TIME_MIN_MSEC <= holdMSecond && holdMSecond <= SHUTDOWN_TIME_MAX_MSEC;
    }

    if (! isSystemShuttingDown && (buttonHeldAndReleased || buttonHeldDownForShutdown)) {
        isSystemShuttingDown = true;
        system("shutdown -h now &");
    }
}

// Returns -1 if error, 0 if timeout, otherwise there is data to read
int timeout_serial_read(int fd) {
    fd_set set;
    struct timeval timeout;

    FD_ZERO(&set);
    FD_SET(fd, &set);
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000 * 10;

    return select(fd + 1, &set, NULL, NULL, &timeout);
}

/*****************************************************************
    Functions below are a one time call for initialization only
*****************************************************************/

/**
 * Initializes the daemon so that mcu.serial would listen in the background
 */
void init_daemon() {
    pid_t process_id = 0;
    pid_t sid = 0;

    // Create child process
    process_id = fork();

    // Indication of fork() failure
    if (process_id < 0) {
        syslog(LOG_ERR, "forking failed");
        exit(1);
    }

    // PARENT PROCESS. Need to kill it.
    if (process_id > 0) {
        syslog(LOG_INFO, "process_id of child process %i", process_id);
        exit(0);
    }

    //unmask the file mode
    umask(0);
    //set new session
    sid = setsid();

    if(sid < 0) {
        syslog(LOG_WARNING, "could not set new session");
        exit(1);
    }

    // Close stdin. stdout and stderr
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}


void init_server(int portno) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        syslog(LOG_ERR, "could not open BSD socket");
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        syslog(LOG_ERR, "could not bind to BSD socket");
        exit(1);
    }
}

void init_serial() {
    fserial = open (SERIAL_PORT, O_RDWR | O_NOCTTY | O_SYNC);
    if (fserial < 0) {
        syslog(LOG_ERR, "could not open serial port");
        exit(1);
    }

    set_interface_attribs (fserial, B115200, 0);
    set_blocking (fserial, 0);
}

// Sets the interface for the port
void set_interface_attribs (int fd, int speed, int parity) {
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)  {
        syslog(LOG_ERR, "error from tcgetattr");
        exit(1);
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (fd, TCSANOW, &tty) != 0) {
        syslog(LOG_ERR, "error %d from tcsetattr", errno);
        exit(1);
    }
}

void set_blocking (int fd, int should_block) {
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0) {
        syslog(LOG_ERR, "error %d from tggetattr", errno);
        exit(1);
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0) {
        syslog(LOG_ERR, "error %d setting term attributes", errno);
        exit(1);
    }
}
