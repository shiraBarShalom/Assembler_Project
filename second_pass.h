/*
 * file: second_pass.h
 * description: gives the external_usage_node struct and the second pass prototypes
 * main uses run_second_pass to start the second pass and output.c uses the external
 * usage list to build the ext file
 */

#ifndef SECOND_PASS_H
#define SECOND_PASS_H

#include "globals.h"
#include "symbol_table.h"
#include "memory_image.h"

/*
 * structure: external_usage_node
 * input: none
 * output: none
 * operation: stores a use of an external symbol for the ext file
 */
typedef struct external_usage_node {
    char name[MAX_LABEL_LEN];
    int instruction_address;
    struct external_usage_node *next;
} external_usage_node;

/*
 * function: run_second_pass
 * input: memory state symbol table pointer to external usages list and memory error flag
 * output: true if successful false otherwise
 * operation: checks entries and completes unresolved references from the first pass
 */
boolean run_second_pass(memory_state *state, symbol_node *symbol_table, external_usage_node **external_usages, boolean *memory_error);

/*
 * function: free_external_usages
 * input: external usages list
 * output: none
 * operation: frees all external usages in the list
 */
void free_external_usages(external_usage_node *head);

#endif