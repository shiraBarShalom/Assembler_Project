/*
 * file: first_pass.h
 * description: gives the prototype for run_first_pass
 * main uses this to run the first pass on the expanded am file
 */

#ifndef FIRST_PASS_H
#define FIRST_PASS_H
#include "globals.h"
#include "symbol_table.h"
#include "memory_image.h"
#include "parser.h"

/*
* function: run_first_pass
* input: file name memory state pointer to the symbol table and memory error flag
* output: true if the first pass finished successfully false otherwise
* operation: reads the file checks each line and builds the symbol table and memory images
*/
boolean run_first_pass(char file_name[], memory_state *state, symbol_node **sym_table, boolean *memory_error);

#endif