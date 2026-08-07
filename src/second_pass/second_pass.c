/* second_pass.c */
#include "second_pass.h"

/* numeric bounds */
#define MAX_16_BIT 32767
#define MIN_16_BIT -32768

/* masks for clearing and setting specific fields */
#define IMMED_MASK 0xFFFFUL
#define CLEAR_IMMED_MASK 0xFFFF0000UL
#define CLEAR_ADDRESS_MASK 0xFE000000UL
#define ADDRESS_MASK 0x01FFFFFFUL

/*
 * function: add_external_usage
 * input: pointer to list head name and address
 * output: true if successful false if memory fails
 * operation: adds external symbol usage to list
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
 * input: entry node symbol table and global error flag
 * output: none
 * operation: validates single entry and updates symbol table flag
 */
static void process_single_entry(entry_node *curr_entry, symbol_node *symbol_table, boolean *has_error) {
    symbol_node *sym = find_symbol(symbol_table, curr_entry->name); /* lookup node */
    
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
 * input: state symbol table and global error flag
 * output: none
 * operation: iterates and processes all entry requests
 */
static void process_all_entries(memory_state *state, symbol_node *symbol_table, boolean *has_error) {
    entry_node *curr_entry = state->entries_head; /* entry list iterator */
    
    while (curr_entry != NULL) {
        process_single_entry(curr_entry, symbol_table, has_error);
        curr_entry = curr_entry->next;
    }
}

/*
 * function: process_branch_reference
 * input: reference node symbol state and global error flag
 * output: none
 * operation: validates branch distance and encodes immediate value
 */
static void process_branch_reference(reference_node *curr_ref, symbol_node *sym, memory_state *state, boolean *has_error) {
    int index = (curr_ref->instruction_address - INITIAL_IC) / 4; /* array location */
    unsigned long word = state->code_image[index].value; /* machine instruction */
    int distance; /* branch offset */

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
 * input: reference node symbol state external usage list and error flag
 * output: none
 * operation: encodes address or records external usage for jump instruction
 */
static void process_jump_reference(reference_node *curr_ref, symbol_node *sym, memory_state *state, external_usage_node **external_usages, boolean *has_error) {
    int index = (curr_ref->instruction_address - INITIAL_IC) / 4; /* array location */
    unsigned long word = state->code_image[index].value; /* machine instruction */

    if (sym->type == SYMBOL_EXTERN) {
        word &= CLEAR_ADDRESS_MASK;
        state->code_image[index].value = word;
        
        if (!add_external_usage(external_usages, sym->name, curr_ref->instruction_address)) {
            printf("error in line %d memory allocation failed\n", curr_ref->line_num);
            *has_error = true;
        }
    } else {
        word &= CLEAR_ADDRESS_MASK;
        word |= ((unsigned long)sym->address & ADDRESS_MASK);
        state->code_image[index].value = word;
    }
}

/*
 * function: process_single_reference
 * input: reference node symbol table state external list and error flag
 * output: none
 * operation: looks up symbol and directs reference processing
 */
static void process_single_reference(reference_node *curr_ref, symbol_node *symbol_table, memory_state *state, external_usage_node **external_usages, boolean *has_error) {
    symbol_node *sym = find_symbol(symbol_table, curr_ref->name); /* lookup node */
    
    if (sym == NULL) {
        printf("error in line %d symbol %s not defined\n", curr_ref->line_num, curr_ref->name);
        *has_error = true;
        return;
    }

    if (curr_ref->type == REF_I_TYPE) {
        process_branch_reference(curr_ref, sym, state, has_error);
    } else if (curr_ref->type == REF_J_TYPE) {
        process_jump_reference(curr_ref, sym, state, external_usages, has_error);
    }
}

/*
 * function: process_all_references
 * input: state symbol table external list and error flag
 * output: none
 * operation: iterates and processes all memory address references
 */
static void process_all_references(memory_state *state, symbol_node *symbol_table, external_usage_node **external_usages, boolean *has_error) {
    reference_node *curr_ref = state->refs_head; /* references list iterator */
    
    while (curr_ref != NULL) {
        process_single_reference(curr_ref, symbol_table, state, external_usages, has_error);
        curr_ref = curr_ref->next;
    }
}

/*
 * function: free_external_usages
 * input: pointer to head of external usages list
 * output: none
 * operation: frees memory of external usages list
 */
void free_external_usages(external_usage_node *head) {
    external_usage_node *current = head; /* list iterator */
    external_usage_node *next_node; /* freeing pointer */

    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }
}

/*
 * function: run_second_pass
 * input: pointer to state symbol table and pointer to externals list
 * output: true if successful false if errors
 * operation: manages full second pass workflow
 */
boolean run_second_pass(memory_state *state, symbol_node *symbol_table, external_usage_node **external_usages) {
    boolean has_error = false; /* tracks if pass failed */

    *external_usages = NULL;

    process_all_entries(state, symbol_table, &has_error);
    process_all_references(state, symbol_table, external_usages, &has_error);

    if (has_error) {
        free_external_usages(*external_usages);
        *external_usages = NULL;
        return false;
    }

    return true;
}