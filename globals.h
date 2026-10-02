/*
 * file: globals.h
 * description: holds the constants and the boolean type used across the project
 * things like the max line length, max label length and max operands are defined
 * here so every file that needs them includes this header
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/*
 * maximum number of characters allowed in one source line
 */
#define MAX_SOURCE_LINE 80

/*
 * line buffer size: 80 characters plus '\n' and '\0'
 */
#define MAX_LINE_LEN 82

/* label buffer size including '\0' */
#define MAX_LABEL_LEN 32

/* maximum number of operands in an instruction */
#define MAX_INSTRUCTION_OPERANDS 3

/* maximum number of operands kept from a data directive */
#define MAX_OPERANDS_PER_LINE 40

/* boolean values used in the project */
typedef enum {
    false,
    true
} boolean;

#endif