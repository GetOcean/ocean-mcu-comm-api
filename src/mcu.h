//
// Created by Kousha Talebian on 4/7/15.
//

#ifndef MCU_MCU_H
#define MCU_MCU_H

    // CPU
    #define CPU_ON 10
    #define CPU_OFF 11
    #define CPU_SHUTOFF 12
    #define CPU_BOOTING 13
    #define CPU_BOOTED 14

    // USB
    #define USB_ON 15
    #define USB_OFF 16

    // STATUS STATUS (CPU/USB)
    #define PWR_STATUS 17

    // Battery state
    #define BATTERY_STATE_ON 20
    #define BATTERY_STATE_OFF 21
    #define BATTERY_LEVEL 22

    // LED_BTN (RGB)
    #define LED_BTN 30
    #define LED_BTN_ON 31
    #define LED_BTN_OFF 32

    // LED_BTN_RED
    #define LED_BTN_RED_ON 33
    #define LED_BTN_RED_OFF 34

    // LED_BTN_GREEN
    #define LED_BTN_GREEN_ON 35
    #define LED_BTN_GREEN_OFF 36

    // LED_BTN_BLUE
    #define LED_BTN_BLUE_ON 37
    #define LED_BTN_BLUE_OFF 38

    // BT button
    #define LED_BT_ON 39
    #define LED_BT_OFF 40

    // RAW COMMAND
    #define RAW_COMMAND 99

    // UNDEFINED COMMAND
    #define BAD_COMMAND -1

    #define BUFFER_RESPONSE_SIZE 32
    #define BUFFER_COMMAND_SIZE 16

    // Functions
    void ledBtn(char *);
    int command(char *);
    int readSocketPort();
    void write_to_client(char *);

    int attempts_to_write;
    #define MAX_ATTEMPTS 2

#endif //MCU_MCU_H
