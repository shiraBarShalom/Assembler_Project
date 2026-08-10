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
 * function: run_second_pass
 * input: pointer to state symbol table and pointer to externals list
 * output: true if successful false if errors
 * operation: updates code image with addresses and checks entries
 */
boolean run_second_pass(memory_state *state, symbol_node *symbol_table, external_usage_node **external_usages) {
    boolean has_error = false; /* tracks if pass failed */
    entry_node *curr_entry = state->entries_head; /* entry list iterator */
    reference_node *curr_ref = state->refs_head; /* references list iterator */
    symbol_node *sym; /* found symbol pointer */
    int distance; /* branch offset */
    int index; /* array location */
    unsigned long word; /* modified instruction */

    *external_usages = NULL;

    while (curr_entry != NULL) {
        sym = find_symbol(symbol_table, curr_entry->name);
        
        if (sym == NULL) {
            printf("error in line %d entry symbol %s not defined\n", curr_entry->line_num, curr_entry->name);
            has_error = true;
        } else if (sym->type == SYMBOL_EXTERN) {
            printf("error in line %d symbol %s cannot be both entry and extern\n", curr_entry->line_num, curr_entry->name);
            has_error = true;
        } else {
            sym->is_entry = true;
        }
        
        curr_entry = curr_entry->next;
    }

    while (curr_ref != NULL) {
        sym = find_symbol(symbol_table, curr_ref->name);
        
        if (sym == NULL) {
            printf("error in line %d symbol %s not defined\n", curr_ref->line_num, curr_ref->name);
            has_error = true;
            curr_ref = curr_ref->next;
            continue;
        }

        index = (curr_ref->instruction_address - INITIAL_IC) / 4;
        word = state->code_image[index].value;

        if (curr_ref->type == REF_I_TYPE) {
            if (sym->type == SYMBOL_EXTERN) {
                printf("error in line %d branch target %s cannot be external\n", curr_ref->line_num, curr_ref->name);
                has_error = true;
            } else {
                distance = sym->address - curr_ref->instruction_address;
                
                if (distance < MIN_16_BIT || distance > MAX_16_BIT) {
                    printf("error in line %d branch distance out of bounds\n", curr_ref->line_num);
                    has_error = true;
                } else {
                    word &= CLEAR_IMMED_MASK;
                    word |= ((unsigned long)distance & IMMED_MASK);
                    state->code_image[index].value = word;
                }
            }
        } else if (curr_ref->type == REF_J_TYPE) {
            if (sym->type == SYMBOL_EXTERN) {
                word &= CLEAR_ADDRESS_MASK;
                state->code_image[index].value = word;
                
                if (!add_external_usage(external_usages, sym->name, curr_ref->instruction_address)) {
                    printf("error in line %d memory allocation failed\n", curr_ref->line_num);
                    has_error = true;
                }
            } else {
                word &= CLEAR_ADDRESS_MASK;
                word |= ((unsigned long)sym->address & ADDRESS_MASK);
                state->code_image[index].value = word;
            }
        }
        
        curr_ref = curr_ref->next;
    }

    if (has_error) {
        free_external_usages(*external_usages);
        *external_usages = NULL;
        return false;
    }

    return true;
}

/*
 * function: free_external_usages
 * input: pointer to head of external usages list
 * output: none
 * operation: frees memory of external usages list
 */
void free_external_usages(external_usage_node *head) {
    external_usage_node *current = head;
    external_usage_node *next_node;

    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }
}