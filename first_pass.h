#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include "globals.h"
#include "symbol_table.h"
#include "memory_image.h"
#include "parser.h"

/*
 * function: run_first_pass
 * input: file name state struct and symbol table
 * output: true if successful false if errors found
 * operation: runs first pass logic
 */
boolean run_first_pass(char file_name[], memory_state *state, symbol_node **sym_table);

#endif