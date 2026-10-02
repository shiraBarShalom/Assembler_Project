/*
 * file: second_pass.c
 * description: this file does the second pass of the assembler
 * it checks the entry requests and goes over the references saved in the first pass
 * it fills in the branch distances and the jump addresses, external symbols get saved
 * in a list so the ext file can be built later
 */

#include "second_pass.h"

/* limits for a signed 16 bit value */
#define MAX_16_BIT 32767
#define MIN_16_BIT -32768

/* masks used when updating instruction fields */
#define IMMED_MASK 0xFFFFUL
#define CLEAR_IMMED_MASK 0xFFFF0000UL
#define CLEAR_ADDRESS_MASK 0xFE000000UL
#define ADDRESS_MASK 0x01FFFFFFUL

/*
 * function: add_external_usage
 * input: pointer to external usages list symbol name and address
 * output: true if successful false if memory allocation fails
 * operation: adds a new use of an external symbol to the list
 */
static boolean add_external_usage(external_usage_node **head, char name[], int address) {
    external_usage_node *new_node = (external_usage_node *)malloc(sizeof(external_usage_node));
    if (new_node == NULL) {
        return false;
    }
    
    strcpy(new_node->name, name);
    new_node->instruction_address = address;
    
    new_node->next = *head;
    *head = new_node;
    
    return true;
}

/*
 * function: process_single_entry
 * input: entry symbol table and error flag
 * output: none
 * operation: checks one entry request and marks the matching symbol as entry
 */
static void process_single_entry(entry_node *curr_entry, symbol_node *symbol_table, boolean *has_error) {
    symbol_node *sym = find_symbol(symbol_table, curr_entry->name); /* matching symbol */
    
    if (sym == NULL) {
        printf("error in line %d entry symbol %s not defined\n", curr_entry->line_num, curr_entry->name);
        *has_error = true;
    } else if (sym->type == SYMBOL_EXTERN) {
        printf("error in line %d symbol %s cannot be both entry and extern\n", curr_entry->line_num, curr_entry->name);
        *has_error = true;
    } else {
        sym->is_entry = true;
    }
}

/*
 * function: process_all_entries
 * input: memory state symbol table and error flag
 * output: none
 * operation: checks all entry requests
 */
static void process_all_entries(memory_state *state, symbol_node *symbol_table, boolean *has_error) {
    entry_node *curr_entry = state->entries_head; /* current entry in the list */
    
    while (curr_entry != NULL) {
        process_single_entry(curr_entry, symbol_table, has_error);
        curr_entry = curr_entry->next;
    }
}

/*
 * function: process_branch_reference
 * input: reference symbol memory state and error flag
 * output: none
 * operation: calculates the branch distance and stores it in the instruction
 */
static void process_branch_reference(reference_node *curr_ref, symbol_node *sym, memory_state *state, boolean *has_error) {
    int index = (curr_ref->instruction_address - INITIAL_IC) / 4; /* position in the code image */
    unsigned long word = state->code_image[index].value; /* current instruction word */
    int distance; /* distance from the instruction to the target */

    if (sym->type == SYMBOL_EXTERN) {
        printf("error in line %d branch target %s cannot be external\n", curr_ref->line_num, curr_ref->name);
        *has_error = true;
    } else {
        distance = sym->address - curr_ref->instruction_address;
        
        if (distance < MIN_16_BIT || distance > MAX_16_BIT) {
            printf("error in line %d branch distance out of bounds\n", curr_ref->line_num);
            *has_error = true;
        } else {
            word &= CLEAR_IMMED_MASK;
            word |= ((unsigned long)distance & IMMED_MASK);
            state->code_image[index].value = word;
        }
    }
}

/*
 * function: process_jump_reference
 * input: reference symbol memory state external usages list error flag and memory error flag
 * output: none
 * operation: stores the jump address or saves a use of an external symbol
 */
static void process_jump_reference(reference_node *curr_ref, symbol_node *sym, memory_state *state, external_usage_node **external_usages, boolean *has_error, boolean *memory_error) {
    int index = (curr_ref->instruction_address - INITIAL_IC) / 4; /* position in the code image */
    unsigned long word = state->code_image[index].value; /* current instruction word */

    if (sym->type == SYMBOL_EXTERN) {
        word &= CLEAR_ADDRESS_MASK;
        state->code_image[index].value = word;
        
        if (!add_external_usage(external_usages, sym->name, curr_ref->instruction_address)) {
            printf("error in line %d memory allocation failed\n", curr_ref->line_num);
            *has_error = true;
            *memory_error = true;
        }
    } else {
        word &= CLEAR_ADDRESS_MASK;
        word |= ((unsigned long)sym->address & ADDRESS_MASK);
        state->code_image[index].value = word;
    }
}

/*
 * function: process_single_reference
 * input: reference symbol table memory state external usages list error flag and memory error flag
 * output: none
 * operation: finds the referenced symbol and handles it by its reference type
 */
static void process_single_reference(reference_node *curr_ref, symbol_node *symbol_table, memory_state *state, external_usage_node **external_usages, boolean *has_error, boolean *memory_error) {
    symbol_node *sym = find_symbol(symbol_table, curr_ref->name); /* referenced symbol */
    
    if (sym == NULL) {
        printf("error in line %d symbol %s not defined\n", curr_ref->line_num, curr_ref->name);
        *has_error = true;
        return;
    }

    if (curr_ref->type == REF_I_TYPE) {
        process_branch_reference(curr_ref, sym, state, has_error);
    } else if (curr_ref->type == REF_J_TYPE) {
        process_jump_reference(curr_ref, sym, state, external_usages, has_error, memory_error);
    }
}

/*
 * function: process_all_references
 * input: memory state symbol table external usages list error flag and memory error flag
 * output: none
 * operation: processes all unresolved references from the first pass
 */
static void process_all_references(memory_state *state, symbol_node *symbol_table, external_usage_node **external_usages, boolean *has_error, boolean *memory_error) {
    reference_node *curr_ref = state->refs_head; /* current reference in the list */
    
    while (curr_ref != NULL) {
        process_single_reference(curr_ref, symbol_table, state, external_usages, has_error, memory_error);
        if (*memory_error) {
            break;
        }
        curr_ref = curr_ref->next;
    }
}

/*
 * function: free_external_usages
 * input: external usages list
 * output: none
 * operation: frees all external usages in the list
 */
void free_external_usages(external_usage_node *head) {
    external_usage_node *current = head; /* current node in the list */
    external_usage_node *next_node; /* next node to free */

    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }
}

/*
 * function: run_second_pass
 * input: memory state symbol table pointer to external usages list and memory error flag
 * output: true if successful false otherwise
 * operation: checks entries and completes unresolved references from the first pass
 */
boolean run_second_pass(memory_state *state, symbol_node *symbol_table, external_usage_node **external_usages, boolean *memory_error) {
    boolean has_error = false; /* true if an error was found during the pass */

    *external_usages = NULL;

    process_all_entries(state, symbol_table, &has_error);
    process_all_references(state, symbol_table, external_usages, &has_error, memory_error);

    if (has_error) {
        free_external_usages(*external_usages);
        *external_usages = NULL;
        return false;
    }

    return true;
}
