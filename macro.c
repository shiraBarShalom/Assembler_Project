/*
 * file: macro.c
 * description: handles the list of macros used by the pre assembler
 * it can add a new macro, add a line of text to a macro body, find a macro by name
 * and free all the macros and their lines when they are not needed anymore
 */

#include "macro.h"

/*
 * function: add_macro
 * input: pointer to macro head and macro name
 * output: pointer to the new macro node or null
 * operation: creates a new macro and adds it to the list
 */
macro_node* add_macro(macro_node **head, char name[]) {
    macro_node *new_macro = (macro_node*)malloc(sizeof(macro_node));
    macro_node *current = *head; /* current macro in the list */
    
    if (new_macro == NULL) {
        return NULL;
    }
    
    strcpy(new_macro->name, name);
    new_macro->lines = NULL;
    new_macro->next = NULL;
    
    if (*head == NULL) {
        *head = new_macro;
    } else {
        /* find the end of the list before adding the new macro */
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_macro;
    }
    
    return new_macro;
}

/*
 * function: add_line_to_macro
 * input: pointer to macro node and line string
 * output: true if successful false if memory allocation fails
 * operation: creates a new line and adds it to the macro
 */
boolean add_line_to_macro(macro_node *macro, char line[]) {
    macro_line *new_line = (macro_line*)malloc(sizeof(macro_line));
    macro_line *current; /* current line in the macro */
    
    if (new_line == NULL) {
        return false;
    }
    
    strcpy(new_line->line, line);
    new_line->next = NULL;
    
    if (macro->lines == NULL) {
        macro->lines = new_line;
    } else {
        current = macro->lines;
        /* keep the lines in the same order they were read */
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_line;
    }
    
    return true;
}

/*
 * function: find_macro
 * input: pointer to macro head and macro name
 * output: pointer to macro node if found null otherwise
 * operation: searches the macro list by name
 */
macro_node* find_macro(macro_node *head, char name[]) {
    macro_node *current = head; /* current macro in the list */
    
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

/*
 * function: free_macros
 * input: pointer to macro head
 * output: none
 * operation: frees all macros and their saved lines
 */
void free_macros(macro_node *head) {
    macro_node *current_macro = head; /* current macro in the list */
    macro_node *next_macro; /* next macro to free */
    macro_line *current_line; /* current line in the macro */
    macro_line *next_line; /* next line to free */
    
    while (current_macro != NULL) {
        current_line = current_macro->lines;

        /* save the next line before freeing the current one */
        while (current_line != NULL) {
            next_line = current_line->next;
            free(current_line);
            current_line = next_line;
        }

        /* save the next macro before freeing the current one */
        next_macro = current_macro->next;
        free(current_macro);
        current_macro = next_macro;
    }
}