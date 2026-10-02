/*
 * file: symbol_table.h
 * description: gives the structs and functions for symbols, references and entries
 * the first pass, second pass and output file all use this to store and look up
 * label names, their addresses and the entry and extern information
 */

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "globals.h"

/* types of symbols stored in the symbol table */
typedef enum {
    SYMBOL_CODE,
    SYMBOL_DATA,
    SYMBOL_EXTERN
} symbol_type;

/* types of unresolved references handled in the second pass */
typedef enum {
    REF_J_TYPE,
    REF_I_TYPE
} ref_type;

/*
 * structure: symbol_node
 * input: none
 * output: none
 * operation: stores a symbol name address type and entry flag
 */
typedef struct symbol_node {
    char name[MAX_LABEL_LEN];
    int address;
    symbol_type type;
    boolean is_entry;
    struct symbol_node *next;
} symbol_node;

/*
 * structure: reference_node
 * input: none
 * output: none
 * operation: stores an unresolved reference and the information needed for the second pass
 */
typedef struct reference_node {
    char name[MAX_LABEL_LEN];
    int instruction_address;
    int line_num;
    ref_type type;
    struct reference_node *next;
} reference_node;

/*
 * structure: entry_node
 * input: none
 * output:none
 * operation:stores an entry request and its source line
 */
typedef struct entry_node {
    char name[MAX_LABEL_LEN];
    int line_num;
    struct entry_node *next;
} entry_node;

/*
 * function: add_symbol
 * input: pointer to symbol table name address and type
 * output: pointer to the new symbol or null
 * operation: creates a new symbol and adds it to the table
 */
symbol_node* add_symbol(symbol_node **head, char name[], int address, symbol_type type);

/*
 * function: add_reference
 * input: pointer to reference list name instruction address line number and type
 * output: true if successful false if memory allocation fails
 * operation: adds a new unresolved reference to the list
 */
boolean add_reference(reference_node **head, char name[], int instruction_address, int line_num, ref_type type);

/*
 * function: add_entry_request
 * input: pointer to entry list name and line number
 * output: true if successful false if memory allocation fails
 * operation: adds a new entry request to the list
 */
boolean add_entry_request(entry_node **head, char name[], int line_num);

/*
 * function: find_symbol
 * input: symbol table and symbol name
 * output: pointer to the symbol if found null otherwise
 * operation: searches for a symbol by name
 */
symbol_node* find_symbol(symbol_node *head, char name[]);

/*
 * function: entry_exists
 * input: entry list and symbol name
 * output: true if the entry exists false otherwise
 * operation: checks if an entry request already exists for the name
 */
boolean entry_exists(entry_node *head, char name[]);

/*
 * function: free_symbol_table
 * input: symbol table
 * output: none
 * operation: frees all symbols in the table
 */
void free_symbol_table(symbol_node *head);

/*
 * function: free_references
 * input: reference list
 * output: none
 * operation: frees all references in the list
 */
void free_references(reference_node *head);

/*
 * function: free_entries
 * input: entry list
 * output: none
 * operation: frees all entries in the list
 */
void free_entries(entry_node *head);

#endif