//
// Created by Kousha Talebian on 4/7/15.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "icracked.mcu.util.h"
#include <syslog.h>

/**
 * Returns the timestamp
 */
char * timestamp() {
    time_t ltime;
    ltime = time(NULL);

    return stripNewLine ((char *) asctime(localtime(&ltime)));
}

/**
 * Trims leading/trailing spaces
 */
char * trim(char *c) {
    char * e = c + strlen(c) - 1;
    while(*c && isspace(*c)) c++;
    while(e > c && isspace(*e)) *e-- = '\0';
    return c;
}

/**
 * Strips new line
 */
char * stripNewLine(char *buffer) {
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[--len] = '\0';
    }

    return buffer;
}

char *addCarriageChar(char *buffer) {
    int size_needed;
    char *result;

    size_needed = snprintf(NULL, 0, "%s%s", buffer, "\r");
    result = malloc(size_needed + 1);
    if (result == NULL) {
        printf("out of memory");
        return buffer;
    } else {
        snprintf(result, size_needed + 1, "%s%s", buffer,"\r");
        return result;
    }
}
