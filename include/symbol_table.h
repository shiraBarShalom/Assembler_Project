#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "globals.h"

/* symbol types */
typedef enum {
    SYMBOL_CODE,
    SYMBOL_DATA,
    SYMBOL_EXTERN
} symbol_type;

/* reference encoding types for second pass */
typedef enum {
    REF_J_TYPE,
    REF_I_TYPE
} ref_type;

/*
 * structure: symbol_node
 * input: none
 * output: none
 * operation: symbol node struct holding name address type and entry flag
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
 * operation: tracks unresolved labels instruction addresses and line number for errors
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
 * output: none
 * operation: tracks explicit entry requests
 */
typedef struct entry_node {
    char name[MAX_LABEL_LEN];
    int line_num;
    struct entry_node *next;
} entry_node;

/*
 * function: add_symbol
 * input: pointer to head name address and type
 * output: pointer to the new node or null
 * operation: adds new symbol to head of list
 */
symbol_node* add_symbol(symbol_node **head, char name[], int address, symbol_type type);

/*
 * function: add_reference
 * input: pointer to head name instruction address line number and type
 * output: true if successful false if allocation fails
 * operation: adds new unresolved reference to list
 */
boolean add_reference(reference_node **head, char name[], int instruction_address, int line_num, ref_type type);

/*
 * function: add_entry_request
 * input: pointer to head name and line number
 * output: true if successful false if allocation fails
 * operation: adds new entry directive request to list
 */
boolean add_entry_request(entry_node **head, char name[], int line_num);

/*
 * function: find_symbol
 * input: head of the list and name
 * output: pointer to the found node or null
 * operation: finds symbol by name
 */
symbol_node* find_symbol(symbol_node *head, char name[]);

/*
 * function: entry_exists
 * input: head of the entry list and name
 * output: true if exists false otherwise
 * operation: checks if entry request is already listed
 */
boolean entry_exists(entry_node *head, char name[]);

/*
 * function: free_symbol_table
 * input: pointer to the head of the list
 * output: none
 * operation: frees symbol table memory
 */
void free_symbol_table(symbol_node *head);

/*
 * function: free_references
 * input: pointer to head of reference list
 * output: none
 * operation: frees all memory allocated for references
 */
void free_references(reference_node *head);

/*
 * function: free_entries
 * input: pointer to head of entry list
 * output: none
 * operation: frees all memory allocated for entries
 */
void free_entries(entry_node *head);

#endif