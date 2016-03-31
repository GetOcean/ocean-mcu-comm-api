 //
// Created by Kousha Talebian on 4/7/15.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <math.h>
#include "mcu.h"
#include "icracked.mcu.util.h"
#include "icracked.mcu.client.h"

// Commands would be
/*
 *  Single Commands
 *      cpuOn
 *      cpuOff
 *      usbOn
 *      usbOff
 *      cpuBooted
 *      cpuBooting
 *      batteryStateOn
 *      batteryStateOff
 *      batteryLevel
 *      ledBtnOn
 *      ledBtnOff
 *      ledBTOn
 *      ledBTOff
 *  Double commands
 *      ledBtn 0xFFFFFF
 *      cpuShutoff timeout
 *      raw "command to write"
 *
 *
 *  There should also be
 *      status cpu, status ubc
 *  to return the status (on or off), but I don't want confuse it
 */

int SOCKET_PORT;

int main(int argc, char *argv[]) {
    // Initialize the logger file
    openlog("mcu.log", LOG_CONS | LOG_PERROR, LOG_USER);

    // Maximum 2 inputs are allowed
    if (argc > 3) {
        syslog(LOG_WARNING, "Too many arguments sent");
        exit (1);
    }
    SOCKET_PORT = readSocketPort();

    // The command
    char *cmd = argv[1];
    attempts_to_write = 0;
    switch(command(cmd)) {
        case LED_BTN_RED_ON:
            write_to_client("led r 255");
            break;

        case LED_BTN_RED_OFF:
            write_to_client("led r 0");
            break;

        case LED_BTN_GREEN_ON:
            write_to_client("led g 255");
            break;

        case LED_BTN_GREEN_OFF:
            write_to_client("led g 0");
            break;

        case LED_BTN_BLUE_ON:
            write_to_client("led b 255");
            break;

        case LED_BTN_BLUE_OFF:
            write_to_client("led b 0");
            break;

        case LED_BTN:
            if (argc == 3) {
                ledBtn(argv[2]);
            } else {
                syslog(LOG_WARNING, "No LED color was provided");
            }
            break;

        case LED_BTN_ON:
            ledBtn("0xFFFFFF");
            break;

        case LED_BTN_OFF:
            ledBtn("0x000000");
            break;

        case LED_BT_ON:
            write_to_client("led t 1");
            break;

        case LED_BT_OFF:
            write_to_client("led t 0");
            break;

        case CPU_ON:
            write_to_client("pwr c 1");
            break;

        case CPU_OFF:
            write_to_client("pwr c");
            break;

        case USB_ON:
            write_to_client("pwr d 1");
            break;

        case USB_OFF:
            write_to_client("pwr d 0");
            break;

        case CPU_BOOTING:
            write_to_client("shw c 1");
            break;

        case CPU_BOOTED:
            write_to_client("shw c 0");
            break;

        case BATTERY_STATE_ON:
            write_to_client("shw b 1");
            break;

        case BATTERY_STATE_OFF:
            write_to_client("shw b 0");
            break;

        case BATTERY_LEVEL:
            write_to_client("batl");
            break;

        case CPU_SHUTOFF:
            if (argc == 3) {
                int t = atoi(argv[2]);
                int n = floor(log10(abs(t))) + 1;
                char shutoffCmd[6 + n];
                sprintf(shutoffCmd,"cpu 0 %i", t);
                write_to_client(shutoffCmd);
                break;
            } else {
                syslog(LOG_WARNING, "`cpuShutoff` command incomplete");
            }
            break;

        case PWR_STATUS:
            if (argc == 3) {
                if (strcmp(argv[2], "cpu") == 0) {
                    write_to_client("pwr c ?");
                } else if (strcmp(argv[2], "usb") == 0) {
                    write_to_client("pwr d ?");
                } else {
                    syslog(LOG_WARNING, "argument `status` incomplete");
                }
            } else {
                syslog(LOG_WARNING, "`pwr` command incomplete");
            }
            break;


        case RAW_COMMAND:
            if (argc == 3) {
                write_to_client(argv[2]);
            } else {
                syslog(LOG_WARNING, "no `raw` command provided");
            }
            break;

        default:
            syslog(LOG_WARNING, "unknown MCU command");
            exit (0);
    }

    return 0;
}

void write_to_client(char *cmd) {
    if ((cmd != NULL) && (cmd[0] == '\0')) {
        // Sanity check We don't want empty commands
    } else {
        char *bufferResponse = malloc(BUFFER_RESPONSE_SIZE * sizeof(char));
        if (bufferResponse == NULL) {
            syslog(LOG_WARNING, "Out of memory");
        } else {
            client(bufferResponse, BUFFER_RESPONSE_SIZE, SOCKET_PORT, cmd);

            // Could not write the command
            if (strstr(bufferResponse, "err") != NULL || strstr(bufferResponse, "Err") != NULL) {
                // This is potentially because of the buffer ring
                // So we should rewrite the command
                attempts_to_write++;
                if (attempts_to_write < MAX_ATTEMPTS) {
                    write_to_client(cmd);
                } else {
                    syslog(LOG_ERR, "%s", bufferResponse);
                }
            }
        }
        free(bufferResponse);
    }
}

/**
 * Writes the command for a RGB color
 */
void ledBtn(char *hexString) {
    int hexValue = (int)strtol(hexString, NULL, 16);
    int r = (int) ((hexValue >> 16) & 0xFF);
    int g = (int) ((hexValue >> 8) & 0xFF);
    int b = (int) ((hexValue) & 0xFF);

    /*  We need to write led COLOR BRIGHTNESS
        COLOR is r, g, or b.
        BRIGHTNESS is between 0 - 255
        Example is `led r 255`
        Size is:
            + 3 (`led` string)
            + 1 (`r`, or `g`, or `b`)
            + length of BRIGHTNESS
            + 2 (for spaces)
            = 6 + (1-3 depending on BRIGHTNESS)
    */

    int rSize = 3;
    if (0 < r && r < 10)  rSize = 1;
    if (9 < r && r < 100) rSize = 2;
    char rStr[3 + rSize + 3];
    sprintf(rStr,"led r %i", r);

    int gSize = 3;
    if (0 < g && g < 10)  gSize = 1;
    if (9 < g && g < 100) gSize = 2;
    char gStr[3 + gSize + 3];
    sprintf(gStr,"led g %i", g);

    int bSize = 3;
    if (0 < b && b < 10)  bSize = 1;
    if (9 < b && b < 100) bSize = 2;
    char bStr[3 + bSize + 3];
    sprintf(bStr,"led b %i", b);

    attempts_to_write = 0;
    write_to_client(rStr);
    attempts_to_write = 0;
    write_to_client(gStr);
    attempts_to_write = 0;
    write_to_client(bStr);
}

/**
 * Converts the command from bash to the API
 */
int command(char *cmd) {
    // LED commands
    if (strcmp(cmd, "ledBtn") == 0) return LED_BTN;
    if (strcmp(cmd, "ledBtnOn") == 0) return LED_BTN_ON;
    if (strcmp(cmd, "ledBtnOff") == 0) return LED_BTN_OFF;
    if (strcmp(cmd, "ledBtnRedOn") == 0) return LED_BTN_RED_ON;
    if (strcmp(cmd, "ledBtnGreenOn") == 0) return LED_BTN_GREEN_ON;
    if (strcmp(cmd, "ledBtnBlueOn") == 0) return LED_BTN_BLUE_ON;
    if (strcmp(cmd, "ledBtnRedOff") == 0) return LED_BTN_RED_OFF;
    if (strcmp(cmd, "ledBtnGreenOff") == 0) return LED_BTN_GREEN_OFF;
    if (strcmp(cmd, "ledBtnBlueOff") == 0) return LED_BTN_BLUE_OFF;
    if (strcmp(cmd, "ledBTOn") == 0) return LED_BT_ON;
    if (strcmp(cmd, "ledBTOff") == 0) return LED_BT_OFF;

    // State events
    if (strcmp(cmd, "cpuOn") == 0) return CPU_ON;
    if (strcmp(cmd, "cpuOff") == 0) return CPU_OFF;
    if (strcmp(cmd, "usbOn") == 0) return USB_ON;
    if (strcmp(cmd, "usbOff") == 0) return USB_OFF;
    if (strcmp(cmd, "cpuBooted") == 0) return CPU_BOOTED;
    if (strcmp(cmd, "cpuBooting") == 0) return CPU_BOOTING;
    if (strcmp(cmd, "cpuShutoff") == 0) return CPU_SHUTOFF;
    if (strcmp(cmd, "batteryStateOn") == 0) return BATTERY_STATE_ON;
    if (strcmp(cmd, "batteryStateOff") == 0) return BATTERY_STATE_OFF;
    if (strcmp(cmd, "batteryLevel") == 0) return BATTERY_LEVEL;

    // Return the status of CPU or USB
    if (strcmp(cmd, "status") == 0) return PWR_STATUS;

    // Raw command
    if (strcmp(cmd, "raw") == 0) return RAW_COMMAND;

    return BAD_COMMAND;
}

/**
 * Reads the socket port number from the file
 */
int readSocketPort() {
    FILE *fp = fopen("/tmp/icracked.mcu.port", "r");
    char buffer[BUFSIZ] = {'\0'};

    if (fp != NULL) {
        while (fgets(buffer, BUFSIZ, fp) != NULL) {
            break;
        }
        fclose(fp);
    } else {
        syslog(LOG_ERR, "could not open mcu.port");
    }

    return atoi(buffer);
}
