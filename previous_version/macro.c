#include "macro.h"

/*
 * function: add_macro
 * input: pointer to macro head and macro name
 * output: pointer to the newly created macro node
 * operation: allocates memory for a new macro adds it to the list and returns it
 */
macro_node* add_macro(macro_node **head, char name[]) {
    macro_node *new_macro = (macro_node*)malloc(sizeof(macro_node));
    macro_node *current = *head;
    
    if (new_macro == NULL) {
        printf("memory allocation failed for new macro\n");
        exit(1);
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
 * output: none
 * operation: allocates memory for a new line and adds it to the end of the macro lines
 */
void add_line_to_macro(macro_node *macro, char line[]) {
    macro_line *new_line = (macro_line*)malloc(sizeof(macro_line));
    macro_line *current = macro->lines;
    
    if (new_line == NULL) {
        printf("memory allocation failed for new macro line\n");
        exit(1);
    }
    
    strcpy(new_line->line, line);
    new_line->next = NULL;
    
    if (macro->lines == NULL) {
        macro->lines = new_line;
    } else {
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_line;
    }
}

/*
 * function: find_macro
 * input: pointer to macro head and macro name
 * output: pointer to macro node if found null otherwise
 * operation: searches the linked list for a macro by its name
 */
macro_node* find_macro(macro_node *head, char name[]) {
    macro_node *current = head;
    
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
    macro_node *current_macro = head;
    macro_node *next_macro;
    macro_line *current_line;
    macro_line *next_line;
    
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