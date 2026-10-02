/* 
 * file: first_pass.c
 * description: this file does the first pass of the assembler
 * it reads every line of the am file, finds labels, checks directives and instructions
 * builds the code and data images, fills the symbol table and saves the references
 * that still need to be fixed in the second pass, it also checks the memory size limit
 */

#include "first_pass.h"

#define MAX_16_BIT 32767
#define MIN_16_BIT -32768
#define MAX_32_BIT 2147483647L
#define MIN_32_BIT (-2147483647L - 1L)
#define MAX_8_BIT 127
#define MIN_8_BIT -128

/* bit positions and masks used to build a 32 bit instruction word */
#define OPCODE_SHIFT 26
#define RS_SHIFT 21
#define RT_SHIFT 16
#define RD_SHIFT 11
#define FUNCT_SHIFT 6
#define JMP_REG_FLAG (1UL << 25)
#define IMMED_MASK 0xFFFFUL
#define REG_J_MASK 0x1FFFFFFUL
#define BYTE_MASK 0xFFUL
#define BYTE_SHIFT_8 8
#define BYTE_SHIFT_16 16
#define BYTE_SHIFT_24 24
/* internal helper function declarations*/
static void update_data_symbols(symbol_node *head, int final_ic);
static boolean ensure_code_capacity(memory_state *state);
static boolean ensure_data_capacity(memory_state *state, int extra);

/*
 * function: validate_asciz_quotes
 * input: line start index end index and line number
 * output: true if quotes are valid false otherwise
 * operation: finds the opening and closing quotes of an asciz string
 */
static boolean validate_asciz_quotes(char line[], int *j, int *end_idx, int line_num) {
    skip_spaces(line, j);
    if (line[*j] != '"') {
        printf("error in line %d missing opening quote for string\n", line_num);
        return false;
    }

    (*j)++;
    *end_idx = *j; /* start afterthe opening quote */
    while (line[*end_idx] != '"' && line[*end_idx] != '\0' && line[*end_idx] != '\n') {
        (*end_idx)++;
    }

    if (line[*end_idx] != '"') {
        printf("error in line %d missing closing quote for string\n", line_num);
        return false;
    }
    return true;
}

/*
 * function: validate_asciz_content
 * input: line start index end index and line number
 * output: true if the string is valid false otherwise
 * operation: checks the string content and makes sure nothing follows it
 */
static boolean validate_asciz_content(char line[], int j, int end_idx, int line_num) {
    int temp_j = j; /* position used while checking the string */
    int k = end_idx+1; /* position after the closing quote */

    skip_spaces(line, &k);
    if (line[k] != '\0' && line[k] != '\n') {
        printf("error in line %d extraneous text after string\n", line_num);
        return false;
    }

    while (temp_j < end_idx) {
        if (!isprint((unsigned char)line[temp_j])) {
            printf("error in line %d non printable character in string\n", line_num);
            return false;
        }
        temp_j++;
    }
    return true;
}

/*
 * function: store_asciz_string
 * input: line start index end index memory state error flag line number and memory error flag
 * output: none
 * operation: stores the asciz string in the data image
 */
static void store_asciz_string(char line[], int j, int end_idx, memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    if (!ensure_data_capacity(state, (end_idx - j) + 1)) { /* +1 for '\0' at the end of the string */
        printf("error in line %d data memory allocation failed\n", line_num);
        *line_err = true;
        *memory_error = true;
        return;
    }

    while (j < end_idx) {
        state->data_image[state->dc].value = (unsigned char)line[j];
        state->data_image[state->dc].address = state->dc;
        state->dc++;
        j++;
    }

    /* add '\0' after the last character */
    state->data_image[state->dc].value = 0;
    state->data_image[state->dc].address = state->dc;
    state->dc++;
}

/*
 * function: process_asciz_directive
 * input: line index memory state error flag line number and memory error flag
 * output: none
 * operation: checks and stores an asciz directive
 */
static void process_asciz_directive(char line[], int *index, memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    int j = *index;
    int end_idx;

    if (!validate_asciz_quotes(line, &j, &end_idx, line_num)) {
        *line_err = true;
        return;
    }

    if (!validate_asciz_content(line, j, end_idx, line_num)) {
        *line_err = true;
        return;
    }

    store_asciz_string(line, j, end_idx, state, line_err, line_num, memory_error);
}

/*
 * function: store_db_value
 * input: numeric value memory state error flag line number and memory error flag
 * output: none
 * operation: stores one db value in the data image
 */
static void store_db_value(long num_val, memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    /* .db values must fit in 8 signed bits */
    if (num_val < MIN_8_BIT || num_val > MAX_8_BIT) {
        printf("error in line %d value out of bounds for 8 bit\n", line_num);
        *line_err = true;
    } else if (ensure_data_capacity(state, 1)) {
        state->data_image[state->dc].value = (unsigned char)((unsigned long)num_val & BYTE_MASK); /* keep only the lowest byte */
        state->data_image[state->dc].address = state->dc;
        state->dc += 1;
    } else {
        printf("error in line %d data memory allocation failed\n", line_num);
        *line_err = true;
        *memory_error = true;
    }
}

/*
 * function: store_dh_value
 * input: numeric value memory state error flag line number and memory error flag
 * output: none
 * operation: stores one dh value in the data image
 */
static void store_dh_value(long num_val, memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    unsigned long encoded_val; /* value used for splitting into bytes */

    /* .dh values must fit in 16 signed bits */
    if (num_val < MIN_16_BIT || num_val > MAX_16_BIT) {
        printf("error in line %d value out of bounds for 16 bit\n", line_num);
        *line_err = true;
    } else if (ensure_data_capacity(state, 2)) {
        encoded_val = (unsigned long)num_val;

        /* store the low byte first */
        state->data_image[state->dc].value = (unsigned char)(encoded_val & BYTE_MASK);
        state->data_image[state->dc].address = state->dc;
        state->data_image[state->dc + 1].value = (unsigned char)((encoded_val >> BYTE_SHIFT_8) & BYTE_MASK);
        state->data_image[state->dc + 1].address = state->dc + 1;
        state->dc += 2;
    } else {
        printf("error in line %d data memory allocation failed\n", line_num);
        *line_err = true;
        *memory_error = true;
    }
}

/*
 * function: store_dw_value
 * input: numeric value memory state error flag line number and memory error flag
 * output: none
 * operation: stores one dw value in the data image
 */
static void store_dw_value(long num_val, memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    unsigned long encoded_val; /* value used for splitting into bytes */

    /* .dw values must fit in 32 signed bits */
    if (num_val < MIN_32_BIT || num_val > MAX_32_BIT) {
        printf("error in line %d value out of bounds for 32 bit\n", line_num);
        *line_err = true;
    } else if (ensure_data_capacity(state, 4)) {
        encoded_val = (unsigned long)num_val;

        /* store the bytes from lowest to highest */
        state->data_image[state->dc].value = (unsigned char)(encoded_val&BYTE_MASK);
        state->data_image[state->dc].address = state->dc;
        state->data_image[state->dc + 1].value = (unsigned char)((encoded_val >> BYTE_SHIFT_8) & BYTE_MASK);
        state->data_image[state->dc + 1].address = state->dc + 1;
        state->data_image[state->dc + 2].value = (unsigned char)((encoded_val >> BYTE_SHIFT_16) & BYTE_MASK);
        state->data_image[state->dc + 2].address = state->dc + 2;
        state->data_image[state->dc + 3].value = (unsigned char)((encoded_val >> BYTE_SHIFT_24) & BYTE_MASK);
        state->data_image[state->dc + 3].address = state->dc + 3;
        state->dc += 4;
    } else {
        printf("error in line %d data memory allocation failed\n", line_num);
        *line_err = true;
        *memory_error = true;
    }
}

/*
 * function: process_data_numbers
 * input: line index directive type memory state error flag line number and memory error flag
 * output: none
 * operation: reads numeric operands and stores them by directive type
 */
static void process_data_numbers(char line[], int *index, char dir_type[], memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    char operands[MAX_OPERANDS_PER_LINE][MAX_LINE_LEN];
    int op_count = 0;
    int k;
    long num_val; /* numric value of the current operand */

    if (!extract_operands(line + *index, operands, &op_count, MAX_OPERANDS_PER_LINE)) {
        printf("error in line %d invalid operand structure\n", line_num);
        *line_err = true;
        return;
    }

    if (op_count == 0) {
        printf("error in line %d missing data parameters\n", line_num);
        *line_err = true;
        return;
    }

    for (k = 0; k < op_count; k++) {
        if (!parse_number(operands[k], &num_val)) {
            printf("error in line %d invalid numeric parameter %s\n", line_num, operands[k]);
            *line_err = true;
            break;
        }

        if (strcmp(dir_type, ".db") == 0) {
            store_db_value(num_val, state, line_err, line_num, memory_error);

        } else if (strcmp(dir_type, ".dh") == 0) {
            store_dh_value(num_val, state, line_err, line_num, memory_error);

        } else if (strcmp(dir_type, ".dw") == 0) {
            store_dw_value(num_val, state, line_err, line_num, memory_error);

        }

        if (*line_err) break; /* stop processing the line after an error */
    }
}

/*
 * function: handle_directive
 * input: line index memory state error flag line number and memory error flag
 * output: none
 * operation: handles a data directive by its type
 */
static void handle_directive(char line[], int *index, memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    char dir_type[MAX_LINE_LEN];

    get_first_word(line+ *index, dir_type);

    /* move past the directive name to its operands */
    while (line[*index]!= ' ' && line[*index]!= '\t' && line[*index]!= '\0') {
        (*index)++;
    }

    if (strcmp(dir_type, ".asciz") == 0) {
        process_asciz_directive(line, index, state, line_err, line_num, memory_error);

    } else {
        process_data_numbers(line, index, dir_type, state, line_err, line_num, memory_error);
    }
}

/*
 * function: write_code_word
 * input: instruction word memory state error flag line number and memory error flag
 * output: none
 * operation: stores an encoded instruction in the code image
 */
static void write_code_word(unsigned long word, memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    int code_idx = (state->ic - INITIAL_IC)/4; /* position of the instruction in the code image */

    if (ensure_code_capacity(state)) {
        state->code_image[code_idx].value = word & 0xFFFFFFFFUL; /* keep only 32 bits */
        state->code_image[code_idx].address = state->ic;
        state->ic += 4;
    } else {
        printf("error in line %d code memory allocation failed\n", line_num);
        *line_err = true;
        *memory_error = true;
    }
}

/*
 * function: process_r_instruction
 * input: operation name opcode funct operands operand count word error flag and line number
 * output: none
 * operation: checks and builds an R type instruction
 */
static void process_r_instruction(char opcode_name[], int opcode, int funct, char operands[][MAX_LINE_LEN], int op_count, unsigned long *word, boolean *line_err, int line_num) {
    int rs=0;
    int rt=0;
    int rd=0;

    /* move mvhi and mvlo use 2 registers instead of 3 */
    if (strcmp(opcode_name, "move") == 0 || strcmp(opcode_name, "mvhi") == 0 || strcmp(opcode_name, "mvlo") == 0) {
        if (op_count != 2) {
            printf("error in line %d expected 2 operands for %s\n", line_num, opcode_name);
            *line_err = true; 
            return;
        }
        if (!parse_register(operands[0], &rs) || !parse_register(operands[1], &rd)) {
            printf("error in line %d invalid register operand\n", line_num);
            *line_err = true; 
            return;
        }
        rt = 0; /* this field is not used by these instructions */
    } else {
        if (op_count != 3) {
            printf("error in line %d expected 3 operands for %s\n", line_num, opcode_name);
            *line_err = true; return;
        }
        if (!parse_register(operands[0], &rs) || !parse_register(operands[1], &rt) || !parse_register(operands[2], &rd)) {
            printf("error in line %d invalid register operand\n", line_num);
            *line_err = true; 
            return;
        }
    }

    /* place each field in its position inside the instruction word */
    *word=((unsigned long)opcode << OPCODE_SHIFT) | ((unsigned long)rs << RS_SHIFT) | ((unsigned long)rt << RT_SHIFT) | ((unsigned long)rd << RD_SHIFT) | ((unsigned long)funct << FUNCT_SHIFT);
}

/*
 * function: process_i_instruction
 * input: operation name opcode operands operand count word memory state error flag line number and memory error flag
 * output: none
 * operation: checks and builds an I type instruction
 */
static void process_i_instruction(char opcode_name[], int opcode, char operands[][MAX_LINE_LEN], int op_count, unsigned long *word, memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    int rs=0;
    int rt=0;
    long immed_val=0;

    /* branch instructions use a label instead of a number */
    if (strcmp(opcode_name, "bne") == 0 || strcmp(opcode_name, "beq") == 0 || strcmp(opcode_name, "blt") == 0 || strcmp(opcode_name, "bgt") == 0) {
        if (op_count != 3) {
            printf("error in line %d expected 3 operands\n", line_num);
            *line_err = true; 
            return;
        }
        if (!parse_register(operands[0], &rs) || !parse_register(operands[1], &rt)) {
            printf("error in line %d invalid register operand\n", line_num);
            *line_err = true; 
            return;
        }
        if (!is_valid_label_name(operands[2]) || is_reserved_word(operands[2])) {
            printf("error in line %d invalid branch target label\n", line_num);
            *line_err = true; 
            return;
        }

        immed_val = 0; /* the real distance is added in the second pass */

        /* save the reference so it can be completed in the second pass */
        if (!add_reference(&(state->refs_head), operands[2], state->ic, line_num, REF_I_TYPE)) {
            printf("error in line %d memory allocation failed for reference\n", line_num);
            *line_err = true;
            *memory_error = true;
            return;
        }

    } else {
        if (op_count != 3) {
            printf("error in line %d expected 3 operands\n", line_num);
            *line_err = true; 
            return;
        }
        if (!parse_register(operands[0], &rs) || !parse_number(operands[1], &immed_val) || !parse_register(operands[2], &rt)) {
            printf("error in line %d invalid operand types\n", line_num);
            *line_err = true; return;
        }
        if (immed_val < MIN_16_BIT || immed_val > MAX_16_BIT) {
            printf("error in line %d immediate value out of 16 bit range\n", line_num);
            *line_err = true; 
            return;
        }
    }

    /* place the opcode registers and immediate value in the instruction word */
    *word = ((unsigned long)opcode << OPCODE_SHIFT) | ((unsigned long)rs << RS_SHIFT) | ((unsigned long)rt << RT_SHIFT) | ((unsigned long)immed_val & IMMED_MASK);
}

/*
 * function: process_j_instruction
 * input: operation name opcode operands operand count word memory state error flag line number and memory error flag
 * output: none
 * operation: checks and builds a J type instruction
 */
static void process_j_instruction(char opcode_name[], int opcode, char operands[][MAX_LINE_LEN], int op_count, unsigned long *word, memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    int rs = 0;

    if (strcmp(opcode_name, "hlt") == 0) {
        if (op_count != 0) {
            printf("error in line %d expected 0 operands\n", line_num);
            *line_err = true; 
            return;
        }
        *word = ((unsigned long)opcode << OPCODE_SHIFT);
    }else if (strcmp(opcode_name, "jmp") == 0) {
        if (op_count != 1) {
            printf("error in line %d expected 1 operand\n", line_num);
            *line_err = true; 
            return;
        }

        /* jmp can receive either register or a label */
        if (parse_register(operands[0], &rs)) {
            *word = ((unsigned long)opcode << OPCODE_SHIFT) | JMP_REG_FLAG | ((unsigned long)rs & REG_J_MASK);
        } else if (is_valid_label_name(operands[0]) && !is_reserved_word(operands[0])) {
            *word = ((unsigned long)opcode << OPCODE_SHIFT);

            /* save the label reference for the second pass */
            if (!add_reference(&(state->refs_head), operands[0], state->ic, line_num, REF_J_TYPE)) {
                printf("error in line %d memory allocation failed for reference\n", line_num);
                *line_err = true;
                *memory_error = true;
                return;
            }
        } else {
            printf("error in line %d invalid jmp operand\n", line_num);
            *line_err = true;
            return;
        }
    } else {
        if (op_count != 1) {
            printf("error in line %d expected 1 operand\n", line_num);
            *line_err = true; 
            return;
        }
        if (!is_valid_label_name(operands[0]) || is_reserved_word(operands[0])) {
            printf("error in line %d invalid target label\n", line_num);
            *line_err = true; 
            return;
        }
        *word=((unsigned long)opcode << OPCODE_SHIFT);

        /* save the label reference for the second pass */
        if (!add_reference(&(state->refs_head), operands[0], state->ic, line_num, REF_J_TYPE)) {
            printf("error in line %d memory allocation failed for reference\n", line_num);
            *line_err = true;
            *memory_error = true;
            return;
        }
    }
}

/*
 * function: handle_instruction
 * input: line index operation name memory state error flag line number and memory error flag
 * output: none
 * operation: checks and builds an instruction by its type
 */
static void handle_instruction(char line[], int *index, char opcode_name[], memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    char operands[MAX_INSTRUCTION_OPERANDS][MAX_LINE_LEN];
    int op_count = 0;
    int type;
    int opcode;
    int funct;
    unsigned long word=0;

    if (!get_op_info(opcode_name, &type, &opcode, &funct)) {
        printf("error in line %d unknown instruction %s\n", line_num, opcode_name);
        *line_err = true;
        return;
    }

    /* move past the operation name to its operands */
    while (line[*index] != ' ' && line[*index] != '\t' && line[*index] != '\0') {
        (*index)++;
    }

    if (!check_comma_syntax(line + *index)) {
        printf("error in line %d invalid comma placement\n", line_num);
        *line_err = true;
        return;
    }

    if (!extract_operands(line + *index, operands, &op_count, MAX_INSTRUCTION_OPERANDS)) {
        printf("error in line %d invalid instruction operands format\n", line_num);
        *line_err = true;
        return;
    }

    /* type 1 is R type 2 is I type and 3 is J type */
    if (type == 1) {
        process_r_instruction(opcode_name, opcode, funct, operands, op_count, &word, line_err, line_num);
    } else if (type == 2) {
        process_i_instruction(opcode_name, opcode, operands, op_count, &word, state, line_err, line_num, memory_error);
    } else if (type == 3) {
        process_j_instruction(opcode_name, opcode, operands, op_count, &word, state, line_err, line_num, memory_error);
    }

    if (!*line_err) {
        write_code_word(word, state, line_err, line_num, memory_error);
    }
}

/*
 * function: check_line_exceeds_length
 * input: line file and line number
 * output: true if the line is too long false otherwise
 * operation: checks the line length and skips the rest if it is too long
 */
static boolean check_line_exceeds_length(char line[], FILE *file, int line_num) {
    int line_length = strlen(line);

    /* '\n' is not part of the source line length */
    if (line_length > 0 && line[line_length - 1] == '\n') {
        line_length--;
    }

    if (line_length > MAX_SOURCE_LINE) {
        printf("error in line %d line exceeds maximum length of %d\n", line_num, MAX_SOURCE_LINE);

        /* skip the rest of a line that did not fit in the buffer */
        if (strchr(line, '\n') == NULL && !feof(file)) {
            /* read and discard the rest of a line that is too long until '\n' is reached  */
            while (fgets(line, MAX_LINE_LEN, file) != NULL && strchr(line, '\n') == NULL) {}
        }
        return true;
    }
    return false;
}

/*
 * function: extract_label
 * input: line index label buffer error flag and line number
 * output: true if a label was found false otherwise
 * operation: gets a label from the beginning of the line
 */
static boolean extract_label(char line[], int *i, char label_name[], boolean *line_err, int line_num) {
    boolean line_has_label = has_label(line, label_name);

    if (line_has_label) {
        skip_spaces(line, i);

        /* move past the label */
        while (line[*i] != ' ' && line[*i] != '\t' && line[*i] != '\0') {
            (*i)++;
        }

        /* a label must be followed by another part of the statement */
        if (line[*i] != ' ' && line[*i] != '\t') {
            printf("error in line %d label must be followed by whitespace and an operation\n", line_num);
            *line_err = true;
            return false;
        }
    }

    skip_spaces(line, i);
    return line_has_label;
}

/*
 * function: process_extern_directive
 * input: symbol name symbol table memory state error flag line number and memory error flag
 * output: none
 * operation: checks and adds an extern symbol
 */
static void process_extern_directive(char operand[], symbol_node **sym_table, memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    symbol_node *existing_sym = find_symbol(*sym_table, operand);

    state->has_extern_directive=true;

    /* a local symbol cannot also be declared as extern */
    if (existing_sym != NULL && existing_sym->type != SYMBOL_EXTERN) {
        printf("error in line %d symbol %s is already defined locally\n", line_num, operand);
        *line_err = true;
    } else if (existing_sym == NULL) {
        if (add_symbol(sym_table, operand, 0, SYMBOL_EXTERN) == NULL) {
            printf("error in line %d memory allocation failed for symbol\n", line_num);
            *line_err = true;
            *memory_error = true;
        }
    }
}

/*
 * function: process_entry_directive
 * input: symbol name memory state error flag line number and memory error flag
 * output: none
 * operation: adds an entry request
 */
static void process_entry_directive(char operand[], memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    state->has_entry_directive=true;

    /* do not save the same entry request twice */
    if (!entry_exists(state->entries_head, operand)) {
        if (!add_entry_request(&(state->entries_head), operand, line_num)) {
            printf("error in line %d memory allocation failed for entry request\n", line_num);
            *line_err = true;
            *memory_error = true;
        }
    }
}

/*
 * function: process_symbol_directive
 * input: first word line index label name label flag symbol table memory state error flag line number and memory error flag
 * output: none
 * operation: handles an entry or extern directive
 */
static void process_symbol_directive(char first_word[], char line[], int *i, char label_name[], boolean line_has_label, symbol_node **sym_table, memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    char operands[MAX_OPERANDS_PER_LINE][MAX_LINE_LEN];
    int op_count;

    /* a label before .extern or .entry is checked but not saved */
    if (line_has_label) {
        if (!is_valid_label_name(label_name) || is_reserved_word(label_name)) {
            printf("error in line %d invalid format or reserved word for label %s\n", line_num, label_name);
            *line_err = true;
            return;
        } else {
            printf("warning in line %d meaningless label before directive\n", line_num);
        }
    }

    /* move past the directive name to its operand */
    while (line[*i] != ' ' && line[*i] != '\t' && line[*i] != '\0') {
        (*i)++;
    }

    if (!extract_operands(line + *i, operands, &op_count, MAX_OPERANDS_PER_LINE)) {
        printf("error in line %d invalid operand structure\n", line_num);
        *line_err = true;
    } 
    else if (op_count != 1) {
        printf("error in line %d expected exactly 1 operand\n", line_num);
        *line_err = true;
    } 
    else if (!is_valid_label_name(operands[0]) || is_reserved_word(operands[0])) {
        printf("error in line %d invalid format or reserved word for label %s\n", line_num, operands[0]);
        *line_err = true;
    } 
    else {
        if (strcmp(first_word, ".extern") == 0) {
            process_extern_directive(operands[0], sym_table, state, line_err, line_num, memory_error);
        } else if (strcmp(first_word, ".entry") == 0) {
            process_entry_directive(operands[0], state, line_err, line_num, memory_error);
        }
    }
}

/*
 * function: process_statement
 * input: first word line index label name label flag symbol table memory state error flag line number and memory error flag
 * output: none
 * operation: handles a data directive or instruction
 */
static void process_statement(char first_word[], char line[], int *i, char label_name[], boolean line_has_label, symbol_node **sym_table, memory_state *state, boolean *line_err, int line_num, boolean *memory_error) {
    int current_ic = state->ic; /* code address before processing the statement */
    int current_dc = state->dc; /* data address before processing the statement */

    if (line_has_label) {
        if (!is_valid_label_name(label_name) || is_reserved_word(label_name) || find_symbol(*sym_table, label_name) != NULL) {
            printf("error in line %d invalid format reserved word or duplicate label %s\n", line_num, label_name);
            *line_err = true;
            line_has_label = false; /* do not add an invalid label to the symbol table */
        }
    }

    if (!*line_err) {
        if (first_word[0] == '.') { /* data directives start with '.' */
            if (strcmp(first_word, ".db") == 0 || strcmp(first_word, ".dh") == 0 || strcmp(first_word, ".dw") == 0 || strcmp(first_word, ".asciz") == 0) {
                handle_directive(line, i, state, line_err, line_num, memory_error);
                if (!*line_err && line_has_label) {
                    if (add_symbol(sym_table, label_name, current_dc, SYMBOL_DATA) == NULL) {
                        printf("error in line %d memory allocation failed for data symbol\n", line_num);
                        *line_err = true;
                        *memory_error = true;
                    }
                }
            } else {
                printf("error in line %d unknown directive %s\n", line_num, first_word);
                *line_err = true;
            }
        } else {
            handle_instruction(line, i, first_word, state, line_err, line_num, memory_error);
            if (!*line_err && line_has_label) {
                if (add_symbol(sym_table, label_name, current_ic, SYMBOL_CODE) == NULL) {
                    printf("error in line %d memory allocation failed for code symbol\n", line_num);
                    *line_err = true;
                    *memory_error = true;
                }
            }
        }
    }
}

/*
 * function: rollback_line_errors
 * input: memory state old ic old dc and old reference list
 * output: none
 * operation: restores the state to what it was before a bad line
 */
static void rollback_line_errors(memory_state *state, int old_ic, int old_dc, reference_node *old_refs) {
    reference_node *temp; /* reference that will be removed */

    state->ic = old_ic;
    state->dc = old_dc;

    /* remove references that were added while processing the bad line */
    while (state->refs_head != old_refs) {
        temp = state->refs_head;
        state->refs_head = state->refs_head->next;
        free(temp);
    }
}

/*
 * function: validate_entries_conflict
 * input: entry list symbol table and error flag
 * output: none
 * operation: checks that a symbol is not both entry and extern
 */
static void validate_entries_conflict(entry_node *entries_head, symbol_node *sym_table, boolean *has_error) {
    entry_node *curr_entry = entries_head; /* current entry in the list */
    symbol_node *existing_sym; /* matching symbol if it exists */

    while (curr_entry != NULL) {
        existing_sym = find_symbol(sym_table, curr_entry->name);

        /* extern symbols cannot also be entry symbols */
        if (existing_sym != NULL && existing_sym->type == SYMBOL_EXTERN) {
            printf("error in line %d symbol %s cannot be both entry and extern\n", curr_entry->line_num, curr_entry->name);
            *has_error = true;
        }
        curr_entry = curr_entry->next;
    }
}

/*
 * function: finalize_data_addresses
 * input: memory state and symbol table
 * output: none
 * operation: adds the final code size to all data addresses
 */
static void finalize_data_addresses(memory_state *state, symbol_node *sym_table) {
    int j;

    /* data addresses start from 0 and get their final position after code size is known */
    update_data_symbols(sym_table, state->ic);
    for (j = 0; j < state->dc; j++) {
        state->data_image[j].address += state->ic;
    }
}

/*
 * function: initialize_memory_state_allocation
 * input: memory state
 * output: true if successful false if memory allocation fails
 * operation: initializes the state and allocates the code and data images
 */
static boolean initialize_memory_state_allocation(memory_state *state) {
    state->ic = INITIAL_IC;
    state->dc = 0;
    state->code_capacity = INITIAL_CAPACITY;
    state->data_capacity = INITIAL_CAPACITY;
    state->refs_head = NULL;
    state->entries_head = NULL;
    state->has_extern_directive = false;
    state->has_entry_directive = false;

    state->code_image = (code_word*)malloc(state->code_capacity * sizeof(code_word));
    state->data_image = (data_byte*)malloc(state->data_capacity * sizeof(data_byte));

    /* if one allocation failed free the other one too */
    if (state->code_image == NULL || state->data_image == NULL) {
        if (state->code_image != NULL) free(state->code_image);
        if (state->data_image != NULL) free(state->data_image);
        state->code_image = NULL;
        state->data_image = NULL;
        return false;
    }
    return true;
}

/*
 * function: run_first_pass
 * input: file name memory state pointer to the symbol table and memory error flag
 * output: true if the first pass finished successfully false otherwise
 * operation: reads the file checks each line and builds the symbol table and memory images
 */
boolean run_first_pass(char file_name[], memory_state *state, symbol_node **sym_table, boolean *memory_error) {
    FILE *am_file;
    char line[MAX_LINE_LEN];
    char label_name[MAX_LINE_LEN];
    char first_word[MAX_LINE_LEN];
    reference_node *old_refs; /* reference list before processing the current line */
    boolean has_error = false; /* true if an error was found during the pass */
    boolean line_err; /* error found in the current line */
    boolean line_has_label;
    int line_num = 0;
    int current_ic; /* ic before processing the current line */
    int current_dc; /* dc before processing the current line */
    int i;

    if (!initialize_memory_state_allocation(state)) {
        printf("error memory allocation failed for system images\n");
        *memory_error = true;
        return false;
    }

    am_file = fopen(file_name, "r");
    if (am_file == NULL) {
        printf("error opening am file %s\n", file_name);

        /* the images were already allocated so free them before returning */
        free(state->code_image); free(state->data_image);
        state->code_image = NULL; state->data_image = NULL;
        return false;
    }

    while (fgets(line, sizeof(line), am_file) != NULL) {
        line_num++;
        i = 0;
        line_err = false;

        if (check_line_exceeds_length(line, am_file, line_num)) {
            has_error = true;
            continue;
        }

        if (is_empty_or_comment(line)) {
            continue;
        }

        line_has_label = extract_label(line, &i, label_name, &line_err, line_num);
        get_first_word(line + i, first_word);

        /* save the current state in case this line has to be rolled back */
        current_ic = state->ic;
        current_dc = state->dc;
        old_refs = state->refs_head;

        /* .extern and .entry are handled separately */
        if (strcmp(first_word, ".extern") == 0 || strcmp(first_word, ".entry") == 0) {
            process_symbol_directive(first_word, line, &i, label_name, line_has_label, sym_table, state, &line_err, line_num, memory_error);
        } else {
            process_statement(first_word, line, &i, label_name, line_has_label, sym_table, state, &line_err, line_num, memory_error);
        }

        if (line_err) {
            has_error = true;
            rollback_line_errors(state, current_ic, current_dc, old_refs);
        }

        if (*memory_error) {
            break;
        }
    }

    validate_entries_conflict(state->entries_head, *sym_table, &has_error);

    /* last used address is ic + dc - 1 */
    if ((unsigned long)state->ic + (unsigned long)state->dc - 1UL > MAX_ADDRESS) {
        printf("error total memory capacity address exceeded limits\n");
        has_error = true;
    }

    if (!has_error) {
        finalize_data_addresses(state, *sym_table);
    }

    fclose(am_file);
    return !has_error;
}

/*
 * function: ensure_code_capacity
 * input: memory state
 * output: true if successful false if memory allocation fails
 * operation: increases the code image size when more space is needed
 */
static boolean ensure_code_capacity(memory_state *state) {
    code_word *new_ptr; /* new pointer returned by realloc */
    int code_idx = (state->ic - INITIAL_IC) / 4; /* position of the next instruction */
    int new_cap = state->code_capacity;

    /* increase the size until the next instruction fits */
    while (code_idx >= new_cap) {
        new_cap *= 2;
    }

    if (new_cap > state->code_capacity) {
        new_ptr = (code_word *)realloc(state->code_image, new_cap * sizeof(code_word));
        if (new_ptr == NULL) 
        {
            return false;
        }
        state->code_image = new_ptr;
        state->code_capacity = new_cap;
    }
    return true;
}

/*
 * function: ensure_data_capacity
 * input: memory state and number of extra bytes
 * output: true if successful false if memory allocation fails
 * operation: increases the data image size when more space is needed
 */
static boolean ensure_data_capacity(memory_state *state, int extra) {
    data_byte *new_ptr; /* new pointer returned by realloc */
    int new_cap = state->data_capacity;

    /* increase the size until all new bytes fit */
    while (state->dc + extra > new_cap) {
        new_cap *= 2;
    }

    if (new_cap > state->data_capacity) {
        new_ptr = (data_byte *)realloc(state->data_image, new_cap * sizeof(data_byte));
        if (new_ptr == NULL) return false;
        state->data_image = new_ptr;
        state->data_capacity = new_cap;
    }
    return true;
}

/*
 * function: update_data_symbols
 * input: symbol table and final ic
 * output: none
 * operation: adds the final ic to all data symbol addresses
 */
static void update_data_symbols(symbol_node *head, int final_ic) {
    symbol_node *curr = head; /* current symbol in the table */

    while (curr != NULL) {
        /* only data symbols need the code size added to their address */
        if (curr->type == SYMBOL_DATA) {
            curr->address += final_ic;
        }
        curr = curr->next;
    }
}