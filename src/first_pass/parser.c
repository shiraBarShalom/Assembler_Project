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
 * operation: struct to hold operation dictionary data locally
 */
typedef struct {
    char name[MAX_OP_NAME];
    int type; 
    int opcode;
    int funct;
} op_info_node;

/*
 * function: has_label
 * input: char array line and char array label buffer
 * output: true if label found false otherwise
 * operation: gets label if exists without truncating
 */
boolean has_label(char line[], char label[]) {
    char first_word[MAX_LINE_LEN]; /* extracted token */
    int len; /* length of extracted word */

    get_first_word(line, first_word);
    len = strlen(first_word);

    if (len > 0 && first_word[len - 1] == ':') {
        strncpy(label, first_word, len - 1);
        label[len - 1] = '\0';
        return true;
    }

    return false;
}

/*
 * function: is_valid_label_name
 * input: char array label
 * output: true if valid false otherwise
 * operation: checks if label is alphanumeric starts with letter and length is ok
 */
boolean is_valid_label_name(char label[]) {
    int i = 1; /* character index */
    size_t len = strlen(label); /* label string length */

    if (len == 0 || len > MAX_LABEL_LEN - 1) {
        return false;
    }

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
 * input: char array line starting after command
 * output: true if syntax is valid false otherwise
 * operation: checks comma rules
 */
boolean check_comma_syntax(char line[]) {
    int i = 0; /* loop index */
    boolean expecting_comma = false; /* tracks expected character type */
    boolean empty = true; /* tracks if string has any content */

    skip_spaces(line, &i);

    if (line[i] == ',') {
        return false;
    }

    while (line[i] != '\0' && line[i] != '\n') {
        empty = false;
        if (line[i] == ',') {
            if (!expecting_comma) {
                return false;
            }
            expecting_comma = false;
            i++;
            skip_spaces(line, &i);
            if (line[i] == '\0' || line[i] == '\n' || line[i] == ',') {
                return false;
            }
        } else {
            expecting_comma = true;
            while (line[i] != ' ' && line[i] != '\t' && line[i] != ',' && line[i] != '\0' && line[i] != '\n') {
                i++;
            }
            skip_spaces(line, &i);
        }
    }

    if (!empty && !expecting_comma) {
        return false;
    }

    return true;
}

/*
 * function: get_op_info
 * input: char array word and pointers for type opcode and funct
 * output: true if valid instruction false otherwise
 * operation: looks up local instruction table for encoding details
 */
boolean get_op_info(char word[], int *type, int *opcode, int *funct) {
    int i; /* loop index */
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
 * input: char array operand and pointer to register number
 * output: true if valid register false otherwise
 * operation: validates register format dollar sign and number
 */
boolean parse_register(char operand[], int *reg_num) {
    int i = 1; /* checking index after dollar sign */

    if (operand[0] != '$') {
        return false;
    }

    if (operand[1] == '\0') {
        return false;
    }

    while (operand[i] != '\0') {
        if (!isdigit((unsigned char)operand[i])) {
            return false;
        }
        i++;
    }

    *reg_num = atoi(operand + 1);

    if (*reg_num < 0 || *reg_num > 31) {
        return false;
    }

    return true;
}

/*
 * function: parse_number
 * input: char array operand and pointer to long value
 * output: true if valid integer false otherwise
 * operation: parses string to long integer safely
 */
boolean parse_number(char operand[], long *value) {
    char *endptr; /* pointer for error checking */
    int i = 0; /* loop index */

    if (operand[0] == '\0') {
        return false;
    }

    if (operand[0] == '-' || operand[0] == '+') {
        i++;
        if (operand[i] == '\0') {
            return false;
        }
    }

    while (operand[i] != '\0') {
        if (!isdigit((unsigned char)operand[i])) {
            return false;
        }
        i++;
    }

    errno = 0;
    *value = strtol(operand, &endptr, 10);

    if (errno != 0 || *endptr != '\0') {
        return false;
    }

    return true;
}

/*
 * function: extract_operands
 * input: char array line operands array count pointer and max ops
 * output: true if split succeeds false on error
 * operation: splits operand string by commas safely up to limit
 */
boolean extract_operands(char line[], char operands[][MAX_LINE_LEN], int *count, int max_ops) {
    int i = 0; /* main string index */
    int op_idx = 0; /* current operand index */
    int char_idx = 0; /* current character index */
    int start; /* trim start index */
    int end; /* trim end index */

    *count = 0;
    skip_spaces(line, &i);

    if (line[i] == '\0' || line[i] == '\n') {
        return true;
    }

    while (line[i] != '\0' && line[i] != '\n') {
        if (line[i] == ',') {
            operands[op_idx][char_idx] = '\0';
            op_idx++;
            char_idx = 0;
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

    for (op_idx = 0; op_idx < *count; op_idx++) {
        start = 0;
        skip_spaces(operands[op_idx], &start);
        
        end = strlen(operands[op_idx]) - 1;
        while (end > start && (operands[op_idx][end] == ' ' || operands[op_idx][end] == '\t')) {
            end--;
        }
        
        operands[op_idx][end + 1] = '\0';
        
        if (start > 0) {
            memmove(operands[op_idx], operands[op_idx] + start, end - start + 2);
        }
    }

    return true;
}