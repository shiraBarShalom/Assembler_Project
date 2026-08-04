#ifndef PARSER_H
#define PARSER_H

#include "globals.h"
#include "utils.h"

/*
 * function: has_label
 * input: char array line and char array label buffer
 * output: true if label found false otherwise
 * operation: gets label if exists without truncating
 */
boolean has_label(char line[], char label[]);

/*
 * function: is_valid_label_name
 * input: char array label
 * output: true if valid false otherwise
 * operation: checks if label is alphanumeric starts with letter and length is ok
 */
boolean is_valid_label_name(char label[]);

/*
 * function: check_comma_syntax
 * input: char array line starting after command
 * output: true if syntax is valid false otherwise
 * operation: checks comma rules
 */
boolean check_comma_syntax(char line[]);

/*
 * function: get_op_info
 * input: char array word and pointers for type opcode and funct
 * output: true if valid instruction false otherwise
 * operation: looks up local instruction table for encoding details
 */
boolean get_op_info(char word[], int *type, int *opcode, int *funct);

/*
 * function: parse_register
 * input: char array operand and pointer to register number
 * output: true if valid register false otherwise
 * operation: validates register format dollar sign and number
 */
boolean parse_register(char operand[], int *reg_num);

/*
 * function: parse_number
 * input: char array operand and pointer to long value
 * output: true if valid integer false otherwise
 * operation: parses string to long integer safely
 */
boolean parse_number(char operand[], long *value);

/*
 * function: extract_operands
 * input: char array line operands array count pointer and max ops
 * output: true if split succeeds false on error
 * operation: splits operand string by commas safely up to limit
 */
boolean extract_operands(char line[], char operands[][MAX_LINE_LEN], int *count, int max_ops);

#endif