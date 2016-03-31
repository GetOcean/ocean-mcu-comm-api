//
// Created by Kousha Talebian on 4/7/15.
//

#ifndef MCU_ICRACKED_MCU_UTIL_H
#define MCU_ICRACKED_MCU_UTIL_H

    // Log leves
    #define LOG_LEVEL_INFO 0
    #define LOG_LEVEL_ERROR 1
    #define LOG_LEVEL_WARN 2
    #define LOG_LEVEL_DEBUG 3

    // Functions
    char *timestamp();

    // Removes \n
    char *stripNewLine(char *);

    // Adds \r
    char *addCarriageChar(char *);

    // Trims spaces
    char *trim(char *);

#endif //MCU_ICRACKED_MCU_UTIL_H
