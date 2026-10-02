/*
 * file: pre_assembler.h
 * description: gives the prototype for run_pre_assembler
 * main uses this function to start the pre assembler stage on a source file
 */

#ifndef PRE_ASSEMBLER_H
#define PRE_ASSEMBLER_H

#include "globals.h"
#include "macro.h"
#include "utils.h"

/*
 * function: run_pre_assembler
 * input: source file name with .as extension and memory error flag
 * output: true if successful false otherwise
 * operation: reads the source file expands macros and creates the am file
 */
boolean run_pre_assembler(char file_name[], boolean *memory_error);

#endif