#include "first_pass.h"

/* numeric bounds */
#define MAX_16_BIT 32767
#define MIN_16_BIT -32768
#define MAX_32_BIT 2147483647L
#define MIN_32_BIT (-2147483647L - 1L)
#define MAX_8_BIT 127
#define MIN_8_BIT -128

/* encoding shifts and masks */
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

/* static helper functions */

/*
 * function: update_data_symbols
 * input: symbol table head and final ic
 * output: none
 * operation: adds final ic to data symbols
 */
static void update_data_symbols(symbol_node *head, int final_ic);

/*
 * function: ensure_code_capacity
 * input: state struct pointer
 * output: true if successful false if malloc failed
 * operation: doubles code array capacity safely
 */
static boolean ensure_code_capacity(memory_state *state);

/*
 * function: ensure_data_capacity
 * input: state struct pointer and extra bytes needed
 * output: true if successful false if malloc failed
 * operation: doubles data array capacity safely
 */
static boolean ensure_data_capacity(memory_state *state, int extra);

/*
 * function: validate_asciz_quotes
 * input: line index start end and line number
 * output: true if quotes are valid false otherwise
 * operation: finds and validates asciz string quotes
 */
static boolean validate_asciz_quotes(char line[], int *j, int *end_idx, int line_num) {
    skip_spaces(line, j);
    if (line[*j] != '"') {
        printf("error in line %d missing opening quote for string\n", line_num);
        return false;
    }

    (*j)++;
    *end_idx = *j;
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
 * output: true if content is printable false otherwise
 * operation: verifies all string characters are printable
 */
static boolean validate_asciz_content(char line[], int j, int end_idx, int line_num) {
    int temp_j = j; /* local iterator */
    int k = end_idx + 1; /* trailing text checker */
    
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
 * input: line start index end index state and line num
 * output: none
 * operation: writes asciz bytes to data image
 */
static void store_asciz_string(char line[], int j, int end_idx, memory_state *state, boolean *line_err, int line_num) {
    if (!ensure_data_capacity(state, (end_idx - j) + 1)) {
        printf("error in line %d data memory allocation failed\n", line_num);
        *line_err = true;
        return;
    }

    while (j < end_idx) {
        state->data_image[state->dc].value = (unsigned char)line[j];
        state->data_image[state->dc].address = state->dc;
        state->dc++;
        j++;
    }
    state->data_image[state->dc].value = 0;
    state->data_image[state->dc].address = state->dc;
    state->dc++;
}

/*
 * function: process_asciz_directive
 * input: line index state error and line num
 * output: none
 * operation: handles parsing and storing of asciz string
 */
static void process_asciz_directive(char line[], int *index, memory_state *state, boolean *line_err, int line_num) {
    int j = *index; /* start index */
    int end_idx; /* end quote index */

    if (!validate_asciz_quotes(line, &j, &end_idx, line_num)) {
        *line_err = true;
        return;
    }

    if (!validate_asciz_content(line, j, end_idx, line_num)) {
        *line_err = true;
        return;
    }

    store_asciz_string(line, j, end_idx, state, line_err, line_num);
}

/*
 * function: store_db_value
 * input: encoded value state line error and line num
 * output: none
 * operation: stores a single byte in data image
 */
static void store_db_value(unsigned long encoded_val, memory_state *state, boolean *line_err, int line_num) {
    long num_val = (long)encoded_val;
    if (num_val < MIN_8_BIT || num_val > MAX_8_BIT) {
        printf("error in line %d value out of bounds for 8 bit\n", line_num);
        *line_err = true;
    } else if (ensure_data_capacity(state, 1)) {
        state->data_image[state->dc].value = (unsigned char)(encoded_val & BYTE_MASK);
        state->data_image[state->dc].address = state->dc;
        state->dc += 1;
    } else {
        printf("error in line %d data memory allocation failed\n", line_num);
        *line_err = true;
    }
}

/*
 * function: store_dh_value
 * input: encoded value state line error and line num
 * output: none
 * operation: stores a half word in data image
 */
static void store_dh_value(unsigned long encoded_val, memory_state *state, boolean *line_err, int line_num) {
    long num_val = (long)encoded_val;
    if (num_val < MIN_16_BIT || num_val > MAX_16_BIT) {
        printf("error in line %d value out of bounds for 16 bit\n", line_num);
        *line_err = true;
    } else if (ensure_data_capacity(state, 2)) {
        state->data_image[state->dc].value = (unsigned char)(encoded_val & BYTE_MASK);
        state->data_image[state->dc].address = state->dc;
        state->data_image[state->dc + 1].value = (unsigned char)((encoded_val >> BYTE_SHIFT_8) & BYTE_MASK);
        state->data_image[state->dc + 1].address = state->dc + 1;
        state->dc += 2;
    } else {
        printf("error in line %d data memory allocation failed\n", line_num);
        *line_err = true;
    }
}

/*
 * function: store_dw_value
 * input: encoded value state line error and line num
 * output: none
 * operation: stores a full word in data image
 */
static void store_dw_value(unsigned long encoded_val, memory_state *state, boolean *line_err, int line_num) {
    long num_val = (long)encoded_val;
    if (num_val < MIN_32_BIT || num_val > MAX_32_BIT) {
        printf("error in line %d value out of bounds for 32 bit\n", line_num);
        *line_err = true;
    } else if (ensure_data_capacity(state, 4)) {
        state->data_image[state->dc].value = (unsigned char)(encoded_val & BYTE_MASK);
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
    }
}

/*
 * function: process_data_numbers
 * input: line index dir type state error and line num
 * output: none
 * operation: extracts operands and delegates storage based on directive type
 */
static void process_data_numbers(char line[], int *index, char dir_type[], memory_state *state, boolean *line_err, int line_num) {
    char operands[MAX_OPERANDS_PER_LINE][MAX_LINE_LEN]; /* extracted parameters */
    int op_count = 0; /* amount of parameters */
    int k; /* loop counter */
    long num_val; /* parsed numeric value */
    unsigned long encoded_val; /* safe unsigned cast for masking */

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

        encoded_val = (unsigned long)num_val;

        if (strcmp(dir_type, ".db") == 0) {
            store_db_value(encoded_val, state, line_err, line_num);
        } else if (strcmp(dir_type, ".dh") == 0) {
            store_dh_value(encoded_val, state, line_err, line_num);
        } else if (strcmp(dir_type, ".dw") == 0) {
            store_dw_value(encoded_val, state, line_err, line_num);
        }
        
        if (*line_err) break;
    }
}

/*
 * function: handle_directive
 * input: line index state error line num
 * output: none
 * operation: orchestrates data directives processing
 */
static void handle_directive(char line[], int *index, memory_state *state, boolean *line_err, int line_num) {
    char dir_type[MAX_LINE_LEN]; /* directive name */

    get_first_word(line + *index, dir_type);

    while (line[*index] != ' ' && line[*index] != '\t' && line[*index] != '\0') {
        (*index)++;
    }

    if (strcmp(dir_type, ".asciz") == 0) {
        process_asciz_directive(line, index, state, line_err, line_num);
    } else {
        process_data_numbers(line, index, dir_type, state, line_err, line_num);
    }
}

/*
 * function: write_code_word
 * input: packed word state error flag and line num
 * output: none
 * operation: writes the final instruction to the code image
 */
static void write_code_word(unsigned long word, memory_state *state, boolean *line_err, int line_num) {
    int code_idx = (state->ic - INITIAL_IC) / 4; /* insertion index */
    
    if (ensure_code_capacity(state)) {
        state->code_image[code_idx].value = word & 0xFFFFFFFFUL;
        state->code_image[code_idx].address = state->ic;
        state->ic += 4;
    } else {
        printf("error in line %d code memory allocation failed\n", line_num);
        *line_err = true;
    }
}

/*
 * function: process_r_instruction
 * input: opcode name opcode funct operands op count word ptr line error and line num
 * output: none
 * operation: validates and encodes R type instructions
 */
static void process_r_instruction(char opcode_name[], int opcode, int funct, char operands[][MAX_LINE_LEN], int op_count, unsigned long *word, boolean *line_err, int line_num) {
    int rs = 0; /* source register */
    int rt = 0; /* target register */
    int rd = 0; /* destination register */

    if (strcmp(opcode_name, "move") == 0 || strcmp(opcode_name, "mvhi") == 0 || strcmp(opcode_name, "mvlo") == 0) {
        if (op_count != 2) {
            printf("error in line %d expected 2 operands for %s\n", line_num, opcode_name);
            *line_err = true; return;
        }
        if (!parse_register(operands[0], &rs) || !parse_register(operands[1], &rd)) {
            printf("error in line %d invalid register operand\n", line_num);
            *line_err = true; return;
        }
        rt = 0;
    } else {
        if (op_count != 3) {
            printf("error in line %d expected 3 operands for %s\n", line_num, opcode_name);
            *line_err = true; return;
        }
        if (!parse_register(operands[0], &rs) || !parse_register(operands[1], &rt) || !parse_register(operands[2], &rd)) {
            printf("error in line %d invalid register operand\n", line_num);
            *line_err = true; return;
        }
    }
    *word = ((unsigned long)opcode << OPCODE_SHIFT) | ((unsigned long)rs << RS_SHIFT) | ((unsigned long)rt << RT_SHIFT) | ((unsigned long)rd << RD_SHIFT) | ((unsigned long)funct << FUNCT_SHIFT);
}

/*
 * function: process_i_instruction
 * input: opcode name opcode operands op count word ptr state line error and line num
 * output: none
 * operation: validates encodes and manages references for I type instructions
 */
static void process_i_instruction(char opcode_name[], int opcode, char operands[][MAX_LINE_LEN], int op_count, unsigned long *word, memory_state *state, boolean *line_err, int line_num) {
    int rs = 0; /* source register */
    int rt = 0; /* target register */
    long immed_val = 0; /* numeric immediate */

    if (strcmp(opcode_name, "bne") == 0 || strcmp(opcode_name, "beq") == 0 || strcmp(opcode_name, "blt") == 0 || strcmp(opcode_name, "bgt") == 0) {
        if (op_count != 3) {
            printf("error in line %d expected 3 operands\n", line_num);
            *line_err = true; return;
        }
        if (!parse_register(operands[0], &rs) || !parse_register(operands[1], &rt)) {
            printf("error in line %d invalid register operand\n", line_num);
            *line_err = true; return;
        }
        if (!is_valid_label_name(operands[2]) || is_reserved_word(operands[2])) {
            printf("error in line %d invalid branch target label\n", line_num);
            *line_err = true; return;
        }
        immed_val = 0;

        if (!add_reference(&(state->refs_head), operands[2], state->ic, line_num, REF_I_TYPE)) {
            printf("error in line %d memory allocation failed for reference\n", line_num);
            *line_err = true; return;
        }

    } else {
        if (op_count != 3) {
            printf("error in line %d expected 3 operands\n", line_num);
            *line_err = true; return;
        }
        if (!parse_register(operands[0], &rs) || !parse_number(operands[1], &immed_val) || !parse_register(operands[2], &rt)) {
            printf("error in line %d invalid operand types\n", line_num);
            *line_err = true; return;
        }
        if (immed_val < MIN_16_BIT || immed_val > MAX_16_BIT) {
            printf("error in line %d immediate value out of 16 bit range\n", line_num);
            *line_err = true; return;
        }
    }
    *word = ((unsigned long)opcode << OPCODE_SHIFT) | ((unsigned long)rs << RS_SHIFT) | ((unsigned long)rt << RT_SHIFT) | ((unsigned long)immed_val & IMMED_MASK);
}

/*
 * function: process_j_instruction
 * input: opcode name opcode operands op count word ptr state line error and line num
 * output: none
 * operation: validates encodes and manages references for J type instructions
 */
static void process_j_instruction(char opcode_name[], int opcode, char operands[][MAX_LINE_LEN], int op_count, unsigned long *word, memory_state *state, boolean *line_err, int line_num) {
    int rs = 0; /* register index */

    if (strcmp(opcode_name, "hlt") == 0) {
        if (op_count != 0) {
            printf("error in line %d expected 0 operands\n", line_num);
            *line_err = true; return;
        }
        *word = ((unsigned long)opcode << OPCODE_SHIFT);
    } else if (strcmp(opcode_name, "jmp") == 0) {
        if (op_count != 1) {
            printf("error in line %d expected 1 operand\n", line_num);
            *line_err = true; return;
        }
        if (parse_register(operands[0], &rs)) {
            *word = ((unsigned long)opcode << OPCODE_SHIFT) | JMP_REG_FLAG | ((unsigned long)rs & REG_J_MASK);
        } else if (is_valid_label_name(operands[0]) && !is_reserved_word(operands[0])) {
            *word = ((unsigned long)opcode << OPCODE_SHIFT);
            if (!add_reference(&(state->refs_head), operands[0], state->ic, line_num, REF_J_TYPE)) {
                printf("error in line %d memory allocation failed for reference\n", line_num);
                *line_err = true; return;
            }
        } else {
            printf("error in line %d invalid jmp operand\n", line_num);
            *line_err = true; return;
        }
    } else {
        if (op_count != 1) {
            printf("error in line %d expected 1 operand\n", line_num);
            *line_err = true; return;
        }
        if (!is_valid_label_name(operands[0]) || is_reserved_word(operands[0])) {
            printf("error in line %d invalid target label\n", line_num);
            *line_err = true; return;
        }
        *word = ((unsigned long)opcode << OPCODE_SHIFT);
        if (!add_reference(&(state->refs_head), operands[0], state->ic, line_num, REF_J_TYPE)) {
            printf("error in line %d memory allocation failed for reference\n", line_num);
            *line_err = true; return;
        }
    }
}

/*
 * function: handle_instruction
 * input: line index opcode state error line num
 * output: none
 * operation: orchestrates instruction parsing and encoding based on type
 */
static void handle_instruction(char line[], int *index, char opcode_name[], memory_state *state, boolean *line_err, int line_num) {
    char operands[MAX_INSTRUCTION_OPERANDS][MAX_LINE_LEN]; /* parsed operands */
    int op_count = 0; /* amount of parsed operands */
    int type; /* instruction format r i j */
    int opcode; /* machine opcode */
    int funct; /* machine funct */
    unsigned long word = 0; /* final packed word */

    if (!get_op_info(opcode_name, &type, &opcode, &funct)) {
        printf("error in line %d unknown instruction %s\n", line_num, opcode_name);
        *line_err = true;
        return;
    }

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

    if (type == 1) {
        process_r_instruction(opcode_name, opcode, funct, operands, op_count, &word, line_err, line_num);
    } else if (type == 2) {
        process_i_instruction(opcode_name, opcode, operands, op_count, &word, state, line_err, line_num);
    } else if (type == 3) {
        process_j_instruction(opcode_name, opcode, operands, op_count, &word, state, line_err, line_num);
    }

    if (!*line_err) {
        write_code_word(word, state, line_err, line_num);
    }
}

/*
 * function: check_line_exceeds_length
 * input: line file pointer and line num
 * output: true if line exceeds max length false otherwise
 * operation: clears buffer and reports error for long lines
 */
static boolean check_line_exceeds_length(char line[], FILE *file, int line_num) {
    int line_length = strlen(line); /* measured length */

    if (line_length > 0 && line[line_length - 1] == '\n') {
        line_length--;
    }

    if (line_length > MAX_SOURCE_LINE) {
        printf("error in line %d line exceeds maximum length of %d\n", line_num, MAX_SOURCE_LINE);
        if (strchr(line, '\n') == NULL && !feof(file)) {
            while (fgets(line, LINE_BUFFER_SIZE, file) != NULL && strchr(line, '\n') == NULL) {}
        }
        return true;
    }
    return false;
}

/*
 * function: extract_label
 * input: line pointer to index label buffer error flag and line num
 * output: true if valid label extracted false otherwise
 * operation: parses label and checks basic syntax
 */
static boolean extract_label(char line[], int *i, char label_name[], boolean *line_err, int line_num) {
    boolean line_has_label = has_label(line, label_name); /* true if label found */
    
    if (line_has_label) {
        skip_spaces(line, i);
        while (line[*i] != ' ' && line[*i] != '\t' && line[*i] != '\0') {
            (*i)++;
        }
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
 * input: operand symbol table state line error and line num
 * output: none
 * operation: registers external symbol
 */
static void process_extern_directive(char operand[], symbol_node **sym_table, memory_state *state, boolean *line_err, int line_num) {
    symbol_node *existing_sym = find_symbol(*sym_table, operand); /* duplicate check */
    
    state->has_extern_directive = true;
    if (existing_sym != NULL && existing_sym->type != SYMBOL_EXTERN) {
        printf("error in line %d symbol %s is already defined locally\n", line_num, operand);
        *line_err = true;
    } else if (existing_sym == NULL) {
        if (add_symbol(sym_table, operand, 0, SYMBOL_EXTERN) == NULL) {
            printf("error in line %d memory allocation failed for symbol\n", line_num);
            *line_err = true;
        }
    }
}

/*
 * function: process_entry_directive
 * input: operand state line error and line num
 * output: none
 * operation: registers entry request
 */
static void process_entry_directive(char operand[], memory_state *state, boolean *line_err, int line_num) {
    state->has_entry_directive = true;
    if (!entry_exists(state->entries_head, operand)) {
        if (!add_entry_request(&(state->entries_head), operand, line_num)) {
            printf("error in line %d memory allocation failed for entry request\n", line_num);
            *line_err = true;
        }
    }
}

/*
 * function: process_symbol_directive
 * input: first word line index has label operands symbol table state error and line num
 * output: none
 * operation: orchestrates entry and extern processing
 */
static void process_symbol_directive(char first_word[], char line[], int *i, boolean line_has_label, symbol_node **sym_table, memory_state *state, boolean *line_err, int line_num) {
    char operands[MAX_OPERANDS_PER_LINE][MAX_LINE_LEN]; /* extracted parameters */
    int op_count; /* operand count */

    if (line_has_label) {
        printf("warning in line %d meaningless label before directive\n", line_num);
    }

    while (line[*i] != ' ' && line[*i] != '\t' && line[*i] != '\0') {
        (*i)++;
    }

    if (!extract_operands(line + *i, operands, &op_count, MAX_OPERANDS_PER_LINE)) {
        printf("error in line %d invalid operand structure\n", line_num);
        *line_err = true;
    } else if (op_count != 1) {
        printf("error in line %d expected exactly 1 operand\n", line_num);
        *line_err = true;
    } else if (!is_valid_label_name(operands[0]) || is_reserved_word(operands[0])) {
        printf("error in line %d invalid format or reserved word for label %s\n", line_num, operands[0]);
        *line_err = true;
    } else {
        if (strcmp(first_word, ".extern") == 0) {
            process_extern_directive(operands[0], sym_table, state, line_err, line_num);
        } else if (strcmp(first_word, ".entry") == 0) {
            process_entry_directive(operands[0], state, line_err, line_num);
        }
    }
}

/*
 * function: process_statement
 * input: first word line index label name has label sym table state error and line num
 * output: none
 * operation: distributes statement parsing to directives or instructions
 */
static void process_statement(char first_word[], char line[], int *i, char label_name[], boolean line_has_label, symbol_node **sym_table, memory_state *state, boolean *line_err, int line_num) {
    int current_ic = state->ic; /* ic before line */
    int current_dc = state->dc; /* dc before line */

    if (line_has_label) {
        if (!is_valid_label_name(label_name) || is_reserved_word(label_name) || find_symbol(*sym_table, label_name) != NULL) {
            printf("error in line %d invalid format reserved word or duplicate label %s\n", line_num, label_name);
            *line_err = true;
            line_has_label = false;
        }
    }

    if (!*line_err) {
        if (first_word[0] == '.') {
            if (strcmp(first_word, ".db") == 0 || strcmp(first_word, ".dh") == 0 || strcmp(first_word, ".dw") == 0 || strcmp(first_word, ".asciz") == 0) {
                handle_directive(line, i, state, line_err, line_num);
                if (!*line_err && line_has_label) {
                    if (add_symbol(sym_table, label_name, current_dc, SYMBOL_DATA) == NULL) {
                        printf("error in line %d memory allocation failed for data symbol\n", line_num);
                        *line_err = true;
                    }
                }
            } else {
                printf("error in line %d unknown directive %s\n", line_num, first_word);
                *line_err = true;
            }
        } else {
            handle_instruction(line, i, first_word, state, line_err, line_num);
            if (!*line_err && line_has_label) {
                if (add_symbol(sym_table, label_name, current_ic, SYMBOL_CODE) == NULL) {
                    printf("error in line %d memory allocation failed for code symbol\n", line_num);
                    *line_err = true;
                }
            }
        }
    }
}

/*
 * function: rollback_line_errors
 * input: state old ic old dc old refs
 * output: none
 * operation: reverts state changes if a line has errors
 */
static void rollback_line_errors(memory_state *state, int old_ic, int old_dc, reference_node *old_refs) {
    reference_node *temp; /* freeing pointer */
    state->ic = old_ic;
    state->dc = old_dc;
    while (state->refs_head != old_refs) {
        temp = state->refs_head;
        state->refs_head = state->refs_head->next;
        free(temp);
    }
}

/*
 * function: validate_entries_conflict
 * input: entries head symbol table and global error flag
 * output: none
 * operation: checks that no entry symbol is also external
 */
static void validate_entries_conflict(entry_node *entries_head, symbol_node *sym_table, boolean *has_error) {
    entry_node *curr_entry = entries_head; /* entry iterator */
    symbol_node *existing_sym; /* lookup pointer */

    while (curr_entry != NULL) {
        existing_sym = find_symbol(sym_table, curr_entry->name);
        if (existing_sym != NULL && existing_sym->type == SYMBOL_EXTERN) {
            printf("error in line %d symbol %s cannot be both entry and extern\n", curr_entry->line_num, curr_entry->name);
            *has_error = true;
        }
        curr_entry = curr_entry->next;
    }
}

/*
 * function: finalize_data_addresses
 * input: state and symbol table
 * output: none
 * operation: offsets data symbols and image elements by total ICF
 */
static void finalize_data_addresses(memory_state *state, symbol_node *sym_table) {
    int j; /* image iterator */
    update_data_symbols(sym_table, state->ic);
    for (j = 0; j < state->dc; j++) {
        state->data_image[j].address += state->ic;
    }
}

/*
 * function: initialize_memory_state_allocation
 * input: state
 * output: true if allocated false otherwise
 * operation: allocates image arrays for state
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

    state->code_image = (code_word *)malloc(state->code_capacity * sizeof(code_word));
    state->data_image = (data_byte *)malloc(state->data_capacity * sizeof(data_byte));

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
 * input: file name state struct and symbol table
 * output: true if successful false if errors found
 * operation: manages full first pass workflow
 */
boolean run_first_pass(char file_name[], memory_state *state, symbol_node **sym_table) {
    FILE *am_file; /* source file */
    char line[LINE_BUFFER_SIZE]; /* line buffer */
    char label_name[MAX_LINE_LEN]; /* parsed label */
    char first_word[MAX_LINE_LEN]; /* first token */
    reference_node *old_refs; /* state snapshot */
    boolean has_error = false; /* global error flag */
    boolean line_err; /* local error flag */
    boolean line_has_label; /* label status */
    int line_num = 0; /* current line number */
    int current_ic; /* snapshot ic */
    int current_dc; /* snapshot dc */
    int i; /* string index */

    if (!initialize_memory_state_allocation(state)) {
        printf("error memory allocation failed for system images\n");
        return false;
    }

    am_file = fopen(file_name, "r");
    if (am_file == NULL) {
        printf("error opening am file %s\n", file_name);
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

        current_ic = state->ic;
        current_dc = state->dc;
        old_refs = state->refs_head;

        if (strcmp(first_word, ".extern") == 0 || strcmp(first_word, ".entry") == 0) {
            process_symbol_directive(first_word, line, &i, line_has_label, sym_table, state, &line_err, line_num);
        } else {
            process_statement(first_word, line, &i, label_name, line_has_label, sym_table, state, &line_err, line_num);
        }

        if (line_err) {
            has_error = true;
            rollback_line_errors(state, current_ic, current_dc, old_refs);
        }
    }

    validate_entries_conflict(state->entries_head, *sym_table, &has_error);

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
 * input: state struct pointer
 * output: true if successful false if malloc failed
 * operation: doubles code array capacity safely
 */
static boolean ensure_code_capacity(memory_state *state) {
    code_word *new_ptr;
    int code_idx = (state->ic - INITIAL_IC) / 4;
    int new_cap = state->code_capacity;

    while (code_idx >= new_cap) {
        new_cap *= 2;
    }

    if (new_cap > state->code_capacity) {
        new_ptr = (code_word *)realloc(state->code_image, new_cap * sizeof(code_word));
        if (new_ptr == NULL) return false;
        state->code_image = new_ptr;
        state->code_capacity = new_cap;
    }
    return true;
}

/*
 * function: ensure_data_capacity
 * input: state struct pointer and extra bytes needed
 * output: true if successful false if malloc failed
 * operation: doubles data array capacity safely
 */
static boolean ensure_data_capacity(memory_state *state, int extra) {
    data_byte *new_ptr;
    int new_cap = state->data_capacity;

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
 * input: symbol table head and final ic
 * output: none
 * operation: adds final ic to data symbols
 */
static void update_data_symbols(symbol_node *head, int final_ic) {
    symbol_node *curr = head; /* iterator */
    while (curr != NULL) {
        if (curr->type == SYMBOL_DATA) {
            curr->address += final_ic;
        }
        curr = curr->next;
    }
}