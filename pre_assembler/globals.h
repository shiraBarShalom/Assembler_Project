#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* maximum line length including newline and null terminator */
#define MAX_LINE_LEN 82 

/* maximum label length defined in project specification */
#define MAX_LABEL_LEN 32 

/* boolean enum definition because ansi c lacks built in boolean */
typedef enum {
    false,
    true
} boolean;

#endif