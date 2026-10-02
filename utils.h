/*
 * file: utils.h
 * description: gives the prototypes for the small helper functions in utils.c
 * other files use these to skip spaces, read words, check comments and reserved
 * words and to make new file names from an existing one
 */

#ifndef UTILS_H
#define UTILS_H

#include "globals.h"

/*
 * function: skip_spaces
 * input: line and pointer to index
 * output: none
 * operation: moves the index past spaces and tabs
 */
void skip_spaces(char line[], int *index);

/*
 * function: get_first_word
 * input: line and buffer for the first word
 * output: none
 * operation: copies the first word from the line
 */
void get_first_word(char line[], char word[]);

/*
 * function: is_empty_or_comment
 * input: source line
 * output: true if the line is empty or a comment false otherwise
 * operation: checks if the line should be ignored
 */
boolean is_empty_or_comment(char line[]);

/*
 * function: is_reserved_word
 * input: word to check
 * output: true if the word is reserved false otherwise
 * operation: checks if the word is an instruction directive or register name
 */
boolean is_reserved_word(char word[]);

/*
 * function: replace_extension
 * input: file name and new extension
 * output: new file name or null if memory allocation fails
 * operation: creates a new file name with the given extension
 */
char *replace_extension(const char file_name[], const char new_extension[]);

#endif