//
// Created by Kousha Talebian on 4/7/15.
//

#ifndef MCU_ICRACKED_MCU_SERIAL_H
#define MCU_ICRACKED_MCU_SERIAL_H

    // Definitions
    #define SHUTDOWN_TIME_MIN_MSEC 500 // ms
    #define SHUTDOWN_TIME_MAX_MSEC 5000 // ms
    #define SHUTDOWN_TIME_HELD_MSEC 1000
    #define SERIAL_PORT "/dev/ttyS2"
    #define BATTERY_FILE "/var/battery"
    #define EVENT_BATTERY "batl"
    #define EVENT_BUTTON "ev b"
    #define EVENT_BYPASS_START "ev s1"
    #define EVENT_BYPASS_STOP "logout"
    #define USB_CONSOLE "/usr/local/bin/usb-console"
    #define USB_CONSOLE_BUFFER_SIZE 128


    // Global Variables
    int sockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int fserial;    // Serial port for MCU
    bool isSystemShuttingDown;
    static char usbConsoleAction[USB_CONSOLE_BUFFER_SIZE];

    // Timer for measuring button Time
    struct timespec btnPressedTime;
    struct timespec btnReleasedTime;
    bool isButtonPressed;


    // Functions
    // Initialises the daemon for listening to MCU
    void init_daemon();
    // Initialize the server/socket for MCU command
    void init_server(int);
    // Initialize the serial port to write to microcontroller
    void init_serial();
    void error(char *);
    // Sets attributes of the serial port
    void set_interface_attribs (int, int, int);
    // Sets the attributes of the serial port
    void set_blocking (int, int);
    // Read response from MCU
    void read_from_mcu();
    // Write to MCU
    void write_to_mcu();
    // Times out serial operation
    int timeout_serial_read(int);
    // Handles button press
    void handle_button_press(char *);
    // Determines if we should shutdown
    void action_shutdown();

#endif //MCU_ICRACKED_MCU_SERIAL_H
