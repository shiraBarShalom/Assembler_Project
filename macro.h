#ifndef MACRO_H
#define MACRO_H

#include "globals.h"

/*
 * structure: macro_line
 * input: none
 * output: none
 * operation: structure to hold a single line of macro content
 */
typedef struct macro_line {
    char line[MAX_LINE_LEN];
    struct macro_line *next;
} macro_line;

/*
 * structure: macro_node
 * input: none
 * output: none
 * operation: structure to hold macro name and its lines
 */
typedef struct macro_node {
    char name[MAX_LABEL_LEN];
    macro_line *lines;
    struct macro_node *next;
} macro_node;

/*
 * function: add_macro
 * input: pointer to macro head and macro name
 * output: pointer to the newly created macro node
 * operation: allocates memory for a new macro adds it to the list and returns it
 */
macro_node* add_macro(macro_node **head, char name[]);

/*
 * function: add_line_to_macro
 * input: pointer to macro node and line string
 * output: none
 * operation: allocates memory for a new line and adds it to the end of the macro lines
 */
void add_line_to_macro(macro_node *macro, char line[]);

/*
 * function: find_macro
 * input: pointer to macro head and macro name
 * output: pointer to macro node if found null otherwise
 * operation: searches the linked list for a macro by its name
 */
macro_node* find_macro(macro_node *head, char name[]);

/*
 * function: free_macros
 * input: pointer to macro head
 * output: none
 * operation: frees all allocated memory for macros and their lines
 */
void free_macros(macro_node *head);

#endif