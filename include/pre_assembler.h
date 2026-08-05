#ifndef PRE_ASSEMBLER_H
#define PRE_ASSEMBLER_H

#include "globals.h"
#include "macro.h"
#include "utils.h"

/*
 * function: run_pre_assembler
 * input: original file name with as extension
 * output: true if successful false if there were file errors or macro errors
 * operation: reads the source file expands macros and writes to a new am file
 */
boolean run_pre_assembler(char file_name[]);

#endif