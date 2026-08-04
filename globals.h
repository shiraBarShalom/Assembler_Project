#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/* logical max characters per line */
#define MAX_SOURCE_LINE 80
/* physical buffer for line reading */
#define LINE_BUFFER_SIZE 82
/* alias kept for modules that reference the line buffer size by its original name */
#define MAX_LINE_LEN LINE_BUFFER_SIZE
/* max label size */
#define MAX_LABEL_LEN 32
/* maximum operands an instruction can have */
#define MAX_INSTRUCTION_OPERANDS 3
/* maximum operands a data directive can have */
#define MAX_DATA_OPERANDS 40

/* true false enum */
typedef enum {
    false,
    true
} boolean;

#endif