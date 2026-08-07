#ifndef SECOND_PASS_H
#define SECOND_PASS_H

#include "globals.h"
#include "symbol_table.h"
#include "memory_image.h"

/*
 * structure: external_usage_node
 * input: none
 * output: none
 * operation: tracks uses of external symbols for the ext file
 */
typedef struct external_usage_node {
    char name[MAX_LABEL_LEN];
    int instruction_address;
    struct external_usage_node *next;
} external_usage_node;

/*
 * function: run_second_pass
 * input: pointer to state symbol table and pointer to externals list
 * output: true if successful false if errors
 * operation: updates code image with addresses and checks entries
 */
boolean run_second_pass(memory_state *state, symbol_node *symbol_table, external_usage_node **external_usages);

/*
 * function: free_external_usages
 * input: pointer to head of external usages list
 * output: none
 * operation: frees memory of external usages list
 */
void free_external_usages(external_usage_node *head);

#endif