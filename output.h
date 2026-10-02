/*
 * file: output.h
 * description: gives the prototypes for creating and removing the output files
 * main uses these functions after the second pass to write or clean up the ob ent and ext files
 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include "globals.h"
#include "symbol_table.h"
#include "memory_image.h"
#include "second_pass.h"

/*
* function: create_output_files
* input: file name memory state symbol table external usages list and memory error flag
* output: true if successful false otherwise
* operation: creates the output files needed for the input file
*/
boolean create_output_files(const char file_name[], memory_state *state, symbol_node *symbol_table, external_usage_node *external_usages, boolean *memory_error);

/*
* function: remove_output_files
* input: file name and memory error flag
* output: none
* operation: removes old output files for the input file
*/
void remove_output_files(const char file_name[], boolean *memory_error);

#endif