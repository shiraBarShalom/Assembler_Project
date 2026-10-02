/*
 * file: parser.c
 * description: helper functions used to read and check the parts of a line
 * it can find a label, check if a label name is legal, check the commas between operands
 * look up an instruction in the table and read registers and numbers from the text
 * the first pass uses these functions to break a line into its pieces
 */

#include "parser.h"

#define MAX_OP_NAME 10
#define OP_TYPE_R 1
#define OP_TYPE_I 2
#define OP_TYPE_J 3
#define NUM_INSTRUCTIONS 27

/*
 * structure: op_info_node
 * input: none
 * output: none
 * operation: stores the information needed for one instruction
 */
typedef struct {
    char name[MAX_OP_NAME];
    int type; 
    int opcode;
    int funct;
} op_info_node;

/*
 * function: has_label
 * input: source line and label buffer
 * output: true if a label was found false otherwise
 * operation: gets the label from the beginning of the line
 */
boolean has_label(char line[], char label[]) {
    char first_word[MAX_LINE_LEN];
    int len;

    get_first_word(line, first_word);
    len = strlen(first_word);

    /* a label ends with ':' in the first word of the line */
    if (len > 0 && first_word[len - 1] == ':') {
        strncpy(label, first_word, len - 1);
        label[len - 1] = '\0';
        return true;
    }

    return false;
}

/*
 * function: is_valid_label_name
 * input: label name
 * output: true if valid false otherwise
 * operation: checks if the label name follows the label rules
 */
boolean is_valid_label_name(char label[]) {
    int i = 1; /* first character is checked separately */
    size_t len = strlen(label);

    if (len == 0 || len > MAX_LABEL_LEN - 1) {
        return false;
    }

    /* label names must start with a letter */
    if (!isalpha((unsigned char)label[0])) {
        return false;
    }

    while (label[i] != '\0') {
        if (!isalnum((unsigned char)label[i])) {
            return false;
        }
        i++;
    }

    return true;
}

/*
 * function: check_comma_syntax
 * input: operand part of the line
 * output: true if comma syntax is valid false otherwise
 * operation: checks the comma positions between operands
 */
boolean check_comma_syntax(char line[]) {
    int i = 0;
    boolean expecting_comma = false; /* true after reading an operand */
    boolean empty = true;

    skip_spaces(line, &i);

    /* a comma cannot appear before the first operand */
    if (line[i] == ',') {
        return false;
    }

    while (line[i] != '\0' && line[i] != '\n') {
        empty = false;
        if (line[i] == ',') {
            /* reject two commas without an operand between them */
            if (!expecting_comma) {
                return false;
            }
            expecting_comma = false;
            i++;
            skip_spaces(line, &i);

            /* a comma must be followed by another operand */
            if (line[i] == '\0' || line[i] == '\n' || line[i] == ',') {
                return false;
            }
        } else {
            expecting_comma = true;

            /* move to the end of the current operand */
            while (line[i] != ' ' && line[i] != '\t' && line[i] != ',' && line[i] != '\0' && line[i] != '\n') {
                i++;
            }
            skip_spaces(line, &i);
        }
    }

    /* the line cannot end immediately after a comma */
    if (!empty && !expecting_comma) {
        return false;
    }

    return true;
}

/*
 * function: get_op_info
 * input: operation name and pointers for type opcode and funct
 * output: true if the operation was found false otherwise
 * operation: gets the instruction type opcode and funct
 */
boolean get_op_info(char word[], int *type, int *opcode, int *funct) {
    int i;
    const op_info_node op_table[NUM_INSTRUCTIONS] = {
        {"add",  OP_TYPE_R, 0,  1},
        {"sub",  OP_TYPE_R, 0,  2},
        {"and",  OP_TYPE_R, 0,  3},
        {"or",   OP_TYPE_R, 0,  4},
        {"nor",  OP_TYPE_R, 0,  5},
        {"move", OP_TYPE_R, 1,  1},
        {"mvhi", OP_TYPE_R, 1,  2},
        {"mvlo", OP_TYPE_R, 1,  3},
        {"addi", OP_TYPE_I, 10, 0},
        {"subi", OP_TYPE_I, 11, 0},
        {"andi", OP_TYPE_I, 12, 0},
        {"ori",  OP_TYPE_I, 13, 0},
        {"nori", OP_TYPE_I, 14, 0},
        {"bne",  OP_TYPE_I, 15, 0},
        {"beq",  OP_TYPE_I, 16, 0},
        {"blt",  OP_TYPE_I, 17, 0},
        {"bgt",  OP_TYPE_I, 18, 0},
        {"lb",   OP_TYPE_I, 19, 0},
        {"sb",   OP_TYPE_I, 20, 0},
        {"lw",   OP_TYPE_I, 21, 0},
        {"sw",   OP_TYPE_I, 22, 0},
        {"lh",   OP_TYPE_I, 23, 0},
        {"sh",   OP_TYPE_I, 24, 0},
        {"jmp",  OP_TYPE_J, 30, 0},
        {"la",   OP_TYPE_J, 31, 0},
        {"call", OP_TYPE_J, 32, 0},
        {"hlt",  OP_TYPE_J, 63, 0}
    };

    /* search the instruction table for the operation name */
    for (i = 0; i < NUM_INSTRUCTIONS; i++) {
        if (strcmp(word, op_table[i].name) == 0) {
            *type = op_table[i].type;
            *opcode = op_table[i].opcode;
            *funct = op_table[i].funct;
            return true;
        }
    }

    return false;
}

/*
 * function: parse_register
 * input: operand and pointer to register number
 * output: true if the register is valid false otherwise
 * operation: checks the register format and gets its number
 */
boolean parse_register(char operand[], int *reg_num) {
    int i = 1; /* register number starts after '$' */
    int value = 0;

    if (operand[0] != '$') {
        return false;
    }

    if (operand[1] == '\0') {
        return false;
    }

    /* register 0 must be written as $0 without leading zeros */
    if (operand[1] == '0' && operand[2] != '\0') {
        return false;
    }

    while (operand[i] != '\0') {
        if (!isdigit((unsigned char)operand[i])) {
            return false;
        }

        value = value * 10 + (operand[i] - '0');

        /* valid register numbers are 0 through 31 */
        if (value > 31) {
            return false;
        }

        i++;
    }

    *reg_num = value;
    return true;
}

/*
 * function: parse_number
 * input: operand and pointer to numeric value
 * output: true if the number is valid false otherwise
 * operation: checks and converts the operand to a decimal number
 */
boolean parse_number(char operand[], long *value) {
    char *endptr;
    int i = 0;

    if (operand[0] == '\0') {
        return false;
    }

    /* a number may start with '+' or '-' */
    if (operand[0] == '-' || operand[0] == '+') {
        i++;
        if (operand[i] == '\0') {
            return false;
        }
    }

    /* after the optional sign only digits are allowed */
    while (operand[i] != '\0') {
        if (!isdigit((unsigned char)operand[i])) {
            return false;
        }
        i++;
    }

    errno = 0;
    *value = strtol(operand, &endptr, 10);

    /* check that the conversion succeeded and used the whole string */
    if (errno != 0 || *endptr != '\0') {
        return false;
    }

    return true;
}

/*
 * function: extract_operands
 * input: operand line array for operands count pointer and maximum operands
 * output: true if operands were read successfully false otherwise
 * operation: separates the operands by commas and stores them in the array
 */
boolean extract_operands(char line[], char operands[][MAX_LINE_LEN], int *count, int max_ops) {
    int i = 0;
    int op_idx = 0; /* current operand in the array */
    int char_idx = 0; /* current position inside the operand */
    int start;
    int end;

    *count = 0;
    skip_spaces(line, &i);

    /* no text means there are no operands */
    if (line[i] == '\0' || line[i] == '\n') {
        return true;
    }

    while (line[i] != '\0' && line[i] != '\n') {
        if (line[i] == ',') {
            operands[op_idx][char_idx] = '\0';
            op_idx++;
            char_idx = 0;

            /* do not go past the operand array size */
            if (op_idx >= max_ops) {
                return false;
            }
        } else {
            if (char_idx < MAX_LINE_LEN - 1) {
                operands[op_idx][char_idx] = line[i];
                char_idx++;
            }
        }
        i++;
    }

    operands[op_idx][char_idx] = '\0';
    *count = op_idx + 1;

    /* remove spaces from the beginning and end of each operand */
    for (op_idx = 0; op_idx < *count; op_idx++) {
        start = 0;
        skip_spaces(operands[op_idx], &start);

        end = (int)strlen(operands[op_idx]);
        if (end > 0) {
            end--;
        } else {
            end = -1; /* empty operand */
        }

        while (end > start && (operands[op_idx][end] == ' ' || operands[op_idx][end] == '\t')) {
            end--;
        }

        operands[op_idx][end + 1] = '\0';

        /* move the operand to the start of its buffer after removing leading spaces */
        if (start > 0) {
            memmove(operands[op_idx], operands[op_idx] + start, end - start + 2);
        }
    }

    return true;
}