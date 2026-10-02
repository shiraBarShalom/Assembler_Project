/*
 * file: parser.h
 * description: gives the prototypes for the functions in parser.c
 * the first pass uses these to check labels, commas, instructions, registers and numbers
 */

#ifndef PARSER_H
#define PARSER_H

#include "globals.h"
#include "utils.h"

/*
 * function: has_label
 * input: source line and label buffer
 * output: true if a label was found false otherwise
 * operation: gets the label from the beginning of the line
 */
boolean has_label(char line[], char label[]);

/*
 * function: is_valid_label_name
 * input: label name
 * output: true if valid false otherwise
 * operation: checks if the label name follows the label rules
 */
boolean is_valid_label_name(char label[]);

/*
 * function: check_comma_syntax
 * input: operand part of the line
 * output: true if comma syntax is valid false otherwise
 * operation: checks the comma positions between operands
 */
boolean check_comma_syntax(char line[]);

/*
 * function: get_op_info
 * input: operation name and pointers for type opcode and funct
 * output: true if the operation was found false otherwise
 * operation: gets the instruction type opcode and funct
 */
boolean get_op_info(char word[], int *type, int *opcode, int *funct);

/*
 * function: parse_register
 * input: operand and pointer to register number
 * output: true if the register is valid false otherwise
 * operation: checks the register format and gets its number
 */
boolean parse_register(char operand[], int *reg_num);

/*
 * function: parse_number
 * input: operand and pointer to numeric value
 * output: true if the number is valid false otherwise
 * operation: checks and converts the operand to a decimal number
 */
boolean parse_number(char operand[], long *value);

/*
 * function: extract_operands
 * input: operand line array for operands count pointer and maximum operands
 * output: true if operands were read successfully false otherwise
 * operation: separates the operands by commas and stores them in the array
 */
boolean extract_operands(char line[], char operands[][MAX_LINE_LEN], int *count, int max_ops);

#endif