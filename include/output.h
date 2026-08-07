#ifndef OUTPUT_H
#define OUTPUT_H

#include "globals.h"
#include "symbol_table.h"
#include "memory_image.h"
#include "second_pass.h"

/*
 * function: create_output_files
 * input: file name memory state symbol table and external usages
 * output: true on success false on failure
 * operation: creates ob ent and ext files as required by directives
 */
boolean create_output_files(const char file_name[], memory_state *state, symbol_node *symbol_table, external_usage_node *external_usages);

/*
 * function: remove_output_files
 * input: file name
 * output: none
 * operation: removes old output files for the given input file
 */
void remove_output_files(const char file_name[]);

#endif