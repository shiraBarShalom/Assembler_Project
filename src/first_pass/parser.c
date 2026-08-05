#include "parser.h"

/*
 * structure: op_info_node
 * input: none
 * output: none
 * operation: struct to hold operation dictionary data locally
 */
typedef struct {
    char name[10];
    int type; /* 1 r 2 i 3 j */
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
    char first_word[MAX_LINE_LEN];
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
    int i = 1;
    size_t len = strlen(label);

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
    int i = 0;
    boolean expected_comma = false; /* expected comma flag */
    boolean found_comma = false; /* found comma flag */

    skip_spaces(line, &i);
    if (line[i] == ',') {
        return false;
    }

    while (line[i] != '\0' && line[i] != '\n') {
        if (line[i] == ',') {
            if (expected_comma == false) {
                return false;
            }
            expected_comma = false;
            found_comma = true;
        } else if (line[i] != ' ' && line[i] != '\t') {
            if (found_comma == false && expected_comma == true) {
                return false;
            }
            expected_comma = true;
            found_comma = false;

            while (line[i] != ' ' && line[i] != '\t' && line[i] != ',' && line[i] != '\n' && line[i] != '\0') {
                i++;
            }
            continue;
        }
        i++;
    }

    if (found_comma) {
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
    int i; /* loop index for table */
    int num_ops;
    const op_info_node op_table[] = {
        {"add", 1, 0, 1}, {"sub", 1, 0, 2}, {"and", 1, 0, 3}, {"or", 1, 0, 4},
        {"nor", 1, 0, 5}, {"move", 1, 1, 1}, {"mvhi", 1, 1, 2}, {"mvlo", 1, 1, 3},
        {"addi", 2, 10, 0}, {"subi", 2, 11, 0}, {"andi", 2, 12, 0}, {"ori", 2, 13, 0},
        {"nori", 2, 14, 0}, {"bne", 2, 15, 0}, {"beq", 2, 16, 0}, {"blt", 2, 17, 0},
        {"bgt", 2, 18, 0}, {"lb", 2, 19, 0}, {"sb", 2, 20, 0}, {"lw", 2, 21, 0},
        {"sw", 2, 22, 0}, {"lh", 2, 23, 0}, {"sh", 2, 24, 0},
        {"jmp", 3, 30, 0}, {"la", 3, 31, 0}, {"call", 3, 32, 0}, {"hlt", 3, 63, 0}
    };

    num_ops = sizeof(op_table) / sizeof(op_info_node);

    for (i = 0; i < num_ops; i++) {
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
 * operation: validates register format safely with strtol
 */
boolean parse_register(char operand[], int *reg_num) {
    int i = 1;
    long num; /* extracted register long */
    char *endptr; /* pointer for finding end of number */

    if (operand[0] != '$' || operand[1] == '\0') {
        return false;
    }

    while (operand[i] != '\0') {
        if (!isdigit((unsigned char)operand[i])) {
            return false;
        }
        i++;
    }

    errno = 0;
    num = strtol(operand + 1, &endptr, 10);

    if (errno == ERANGE || num < 0 || num > 31) {
        return false;
    }

    *reg_num = (int)num;
    return true;
}

/*
 * function: parse_number
 * input: char array operand and pointer to long value
 * output: true if valid integer false otherwise
 * operation: parses string to long integer safely
 */
boolean parse_number(char operand[], long *value) {
    char *endptr; /* pointer for finding end of number */
    int i = 0;

    skip_spaces(operand, &i);
    if (operand[i] == '+' || operand[i] == '-') {
        i++;
    }
    if (!isdigit((unsigned char)operand[i])) {
        return false;
    }

    errno = 0;
    *value = strtol(operand, &endptr, 10);

    if (errno == ERANGE) {
        return false;
    }

    while (*endptr != '\0') {
        if (*endptr != ' ' && *endptr != '\t' && *endptr != '\n') {
            return false;
        }
        endptr++;
    }

    return true;
}

/*
 * function: extract_operands
 * input: char array line operands array count pointer and max ops
 * output: true if split succeeds false on error
 * operation: splits operand string by commas and trims spaces safely
 */
boolean extract_operands(char line[], char operands[][MAX_LINE_LEN], int *count, int max_ops) {
    int i = 0;
    int op_idx = 0; /* current operand index */
    int char_idx = 0; /* current character index */
    int start;
    int end;

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
        while (end >= start && (operands[op_idx][end] == ' ' || operands[op_idx][end] == '\t')) {
            end--;
        }
        memmove(operands[op_idx], operands[op_idx] + start, end - start + 1);
        operands[op_idx][end - start + 1] = '\0';
    }

    return true;
}