#include "macro.h"

/*
 * function: add_macro
 * input: pointer to macro head and macro name
 * output: pointer to the newly created macro node or null
 * operation: allocates memory for a new macro adds it to the list
 */
macro_node* add_macro(macro_node **head, char name[]) {
    macro_node *new_macro = (macro_node*)malloc(sizeof(macro_node));
    macro_node *current = *head; /* iterator */
    
    if (new_macro == NULL) {
        return NULL;
    }
    
    strcpy(new_macro->name, name);
    new_macro->lines = NULL;
    new_macro->next = NULL;
    
    if (*head == NULL) {
        *head = new_macro;
    } else {
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
 * operation: allocates memory for a new line and adds it to the macro
 */
boolean add_line_to_macro(macro_node *macro, char line[]) {
    macro_line *new_line = (macro_line*)malloc(sizeof(macro_line));
    macro_line *current; /* iterator */
    
    if (new_line == NULL) {
        return false;
    }
    
    strcpy(new_line->line, line);
    new_line->next = NULL;
    
    if (macro->lines == NULL) {
        macro->lines = new_line;
    } else {
        current = macro->lines;
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
 * operation: searches the linked list for a macro by its name
 */
macro_node* find_macro(macro_node *head, char name[]) {
    macro_node *current = head; /* iterator */
    
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
 * operation: frees all allocated memory for macros and their lines
 */
void free_macros(macro_node *head) {
    macro_node *current_macro = head; /* main list iterator */
    macro_node *next_macro; /* temporary pointer */
    macro_line *current_line; /* inner list iterator */
    macro_line *next_line; /* temporary pointer */
    
    while (current_macro != NULL) {
        current_line = current_macro->lines;
        
        while (current_line != NULL) {
            next_line = current_line->next;
            free(current_line);
            current_line = next_line;
        }
        
        next_macro = current_macro->next;
        free(current_macro);
        current_macro = next_macro;
    }
}