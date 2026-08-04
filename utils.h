#ifndef UTILS_H
#define UTILS_H

#include "globals.h"

/*
 * function: skip_spaces
 * input: char array line and pointer to index
 * output: none
 * operation: skips spaces and tabs
 */
void skip_spaces(char line[], int *index);

/*
 * function: get_first_word
 * input: char array line and char array word
 * output: none
 * operation: gets first word until space or newline
 */
void get_first_word(char line[], char word[]);

/*
 * function: is_empty_or_comment
 * input: char array line
 * output: true if empty or comment false otherwise
 * operation: checks for comment or empty line
 */
boolean is_empty_or_comment(char line[]);

/*
 * function: is_reserved_word
 * input: char array word
 * output: true if reserved false otherwise
 * operation: checks if word is a reserved name
 */
boolean is_reserved_word(char word[]);

#endif