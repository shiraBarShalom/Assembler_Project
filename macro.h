/*
 * file: macro.h
 * description: gives the structs and functions used to store macros
 * it has the macro_line and macro_node structs and the prototypes from macro.c
 * the pre assembler uses this to save macro names, their lines and to expand them later
 */

#ifndef MACRO_H
#define MACRO_H

#include "globals.h"

/*
 * structure: macro_line
 * input: none
 * output: none
 * operation: stores one line from a macro
 */
typedef struct macro_line {
    char line[MAX_LINE_LEN];
    struct macro_line *next;
} macro_line;

/*
 * structure: macro_node
 * input: none
 * output: none
 * operation: stores a macro name and its lines
 */
typedef struct macro_node {
    char name[MAX_LABEL_LEN];
    macro_line *lines;
    struct macro_node *next;
} macro_node;

/*
 * function: add_macro
 * input: pointer to macro head and macro name
 * output: pointer to the new macro node or null
 * operation: creates a new macro and adds it to the list
 */
macro_node* add_macro(macro_node **head, char name[]);

/*
 * function: add_line_to_macro
 * input: pointer to macro node and line string
 * output: true if successful false if memory allocation fails
 * operation: creates a new line and adds it to the macro
 */
boolean add_line_to_macro(macro_node *macro, char line[]);

/*
 * function: find_macro
 * input: pointer to macro head and macro name
 * output: pointer to macro node if found null otherwise
 * operation: searches the macro list by name
 */
macro_node* find_macro(macro_node *head, char name[]);

/*
 * function: free_macros
 * input: pointer to macro head
 * output: none
 * operation: frees all macros and their saved lines
 */
void free_macros(macro_node *head);

#endif