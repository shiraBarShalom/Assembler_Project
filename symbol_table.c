/*
 * file: symbol_table.c
 * description: handles the symbol table and the related lists
 * it can add a symbol, a reference or an entry request, search for a symbol by name
 * check if an entry already exists and free all these lists when the file is done
 */

#include "symbol_table.h"

/*
 * function: add_symbol
 * input: pointer to symbol table name address and type
 * output: pointer to the new symbol or null
 * operation: creates a new symbol and adds it to the table
 */
symbol_node* add_symbol(symbol_node **head, char name[], int address, symbol_type type) {
    symbol_node *new_symbol = (symbol_node*)malloc(sizeof(symbol_node));

    if (new_symbol == NULL) {
        return NULL;
    }

    strcpy(new_symbol->name, name);
    new_symbol->address = address;
    new_symbol->type = type;
    new_symbol->is_entry = false; /* becomes true if the symbol is later marked as entry */

    new_symbol->next = *head; /* add the new symbol at the beginning of the list */
    *head = new_symbol;

    return new_symbol;
}

/*
 * function: add_reference
 * input: pointer to reference list name instruction address line number and type
 * output: true if successful false if memory allocation fails
 * operation: adds a new unresolved reference to the list
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
 * input: pointer to entry list name and line number
 * output: true if successful false if memory allocation fails
 * operation: adds a new entry request to the list
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
 * input: symbol table and symbol name
 * output: pointer to the symbol if found null otherwise
 * operation: searches for a symbol by name
 */
symbol_node* find_symbol(symbol_node *head, char name[]) {
    symbol_node *current = head; /* current symbol in the table */

    while (current != NULL) { /* search the list until the symbol is found */
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

/*
 * function: entry_exists
 * input: entry list and symbol name
 * output: true if the entry exists false otherwise
 * operation: checks if an entry request already exists for the name
 */
boolean entry_exists(entry_node *head, char name[]) {
    entry_node *current = head; /* current entry in the list */

    while (current != NULL) { /* avoid adding the same entry request more than once */
        if (strcmp(current->name, name) == 0) {
            return true;
        }
        current = current->next;
    }

    return false;
}

/*
 * function: free_symbol_table
 * input: symbol table
 * output: none
 * operation: frees all symbols in the table
 */
void free_symbol_table(symbol_node *head) {
    symbol_node *current = head;
    symbol_node *next_node; /* next symbol to free */

    while (current != NULL) {
        next_node = current->next; /* save the next node before freeing the current one */
        free(current);
        current = next_node;
    }
}

/*
 * function: free_references
 * input: reference list
 * output: none
 * operation: frees all references in the list
 */
void free_references(reference_node *head) {
    reference_node *current = head;
    reference_node *next_node; /* next reference to free */

    while (current != NULL) {
        next_node = current->next; /* save the next node before freeing the current one */
        free(current);
        current = next_node;
    }
}

/*
 * function: free_entries
 * input: entry list
 * output: none
 * operation: frees all entries in the list
 */
void free_entries(entry_node *head) {
    entry_node *current = head;
    entry_node *next_node; /* next entry to free */

    while (current != NULL) {
        next_node = current->next; /* save the next node before freeing the current one */
        free(current);
        current = next_node;
    }
}