#ifndef UTILS_H
#define UTILS_H

#include "globals.h"

/*
 * function: skip_spaces
 * input: char array line and pointer to index
 * output: none
 * operation: advances the index past any space or tab characters
 */
void skip_spaces(char line[], int *index);

/*
 * function: get_first_word
 * input: char array line and char array word
 * output: none
 * operation: copies characters of the first word into word buffer until space or newline
 */
void get_first_word(char line[], char word[]);

/*
 * function: is_empty_or_comment
 * input: char array line
 * output: true if empty or comment false otherwise
 * operation: checks if the first non space character is end of line or semicolon
 */
boolean is_empty_or_comment(char line[]);

/*
 * function: is_reserved_word
 * input: char array word
 * output: true if the word is a reserved assembly word false otherwise
 * operation: checks if the word is an instruction name directive name or register name
 */
boolean is_reserved_word(char word[]);

#endif