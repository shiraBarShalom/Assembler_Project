#include "symbol_table.h"

/*
 * function: add_symbol
 * input: pointer to head name address and type
 * output: pointer to the new node or null
 * operation: adds new symbol to head of list
 */
symbol_node* add_symbol(symbol_node **head, char name[], int address, symbol_type type) {
    symbol_node *new_symbol = (symbol_node*)malloc(sizeof(symbol_node));

    if (new_symbol == NULL) {
        return NULL;
    }

    strcpy(new_symbol->name, name);
    new_symbol->address = address;
    new_symbol->type = type;
    new_symbol->is_entry = false;

    new_symbol->next = *head;
    *head = new_symbol;

    return new_symbol;
}

/*
 * function: add_reference
 * input: pointer to head name instruction address line number and type
 * output: true if successful false if allocation fails
 * operation: adds new unresolved reference to list
 */
boolean add_reference(reference_node **head, char name[], int instruction_address, int line_num, ref_type type) {
    reference_node *new_ref = (reference_node*)malloc(sizeof(reference_node));

    if (new_ref == NULL) {
        return false;
    }

    strcpy(new_ref->name, name);
    new_ref->instruction_address = instruction_address;
    new_ref->line_num = line_num;
    new_ref->type = type;

    new_ref->next = *head;
    *head = new_ref;

    return true;
}

/*
 * function: add_entry_request
 * input: pointer to head name and line number
 * output: true if successful false if allocation fails
 * operation: adds new entry directive request to list
 */
boolean add_entry_request(entry_node **head, char name[], int line_num) {
    entry_node *new_entry = (entry_node*)malloc(sizeof(entry_node));

    if (new_entry == NULL) {
        return false;
    }

    strcpy(new_entry->name, name);
    new_entry->line_num = line_num;

    new_entry->next = *head;
    *head = new_entry;

    return true;
}

/*
 * function: find_symbol
 * input: head of the list and name
 * output: pointer to the found node or null
 * operation: finds symbol by name
 */
symbol_node* find_symbol(symbol_node *head, char name[]) {
    symbol_node *current = head; /* iterator */

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

/*
 * function: entry_exists
 * input: head of the entry list and name
 * output: true if exists false otherwise
 * operation: checks if entry request is already listed
 */
boolean entry_exists(entry_node *head, char name[]) {
    entry_node *current = head; /* iterator */

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return true;
        }
        current = current->next;
    }

    return false;
}

/*
 * function: free_symbol_table
 * input: pointer to the head of the list
 * output: none
 * operation: frees symbol table memory
 */
void free_symbol_table(symbol_node *head) {
    symbol_node *current = head; /* iterator */
    symbol_node *next_node; /* temporary pointer */

    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }
}

/*
 * function: free_references
 * input: pointer to head of reference list
 * output: none
 * operation: frees all memory allocated for references
 */
void free_references(reference_node *head) {
    reference_node *current = head; /* iterator */
    reference_node *next_node; /* temporary pointer */

    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }
}

/*
 * function: free_entries
 * input: pointer to head of entry list
 * output: none
 * operation: frees all memory allocated for entries
 */
void free_entries(entry_node *head) {
    entry_node *current = head; /* iterator */
    entry_node *next_node; /* temporary pointer */

    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }
}