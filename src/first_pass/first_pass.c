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

/*
 * function: update_data_symbols
 * input: symbol table head and final ic
 * output: none
 * operation: adds final ic to data symbols
 */
static void update_data_symbols(symbol_node *head, int final_ic);

/*
 * function: handle_directive
 * input: line index state error line num
 * output: none
 * operation: parses data directives and dynamically stores bytes with dc addresses
 */
static void handle_directive(char line[], int *index, memory_state *state, boolean *line_err, int line_num);

/*
 * function: handle_instruction
 * input: line index opcode state error line num
 * output: none
 * operation: parses instructions validates operands and dynamically encodes word and address
 */
static void handle_instruction(char line[], int *index, char opcode[], memory_state *state, boolean *line_err, int line_num);

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
 * function: run_first_pass
 * input: file name state struct and symbol table
 * output: true if successful false if errors found
 * operation: runs first pass logic dynamically and tracks references safely
 */
boolean run_first_pass(char file_name[], memory_state *state, symbol_node **sym_table) {
    FILE *am_file; /* source am file */
    char line[LINE_BUFFER_SIZE]; /* line buffer */
    char label_name[MAX_LINE_LEN]; /* buffer for raw label */
    char first_word[MAX_LINE_LEN]; /* first word in line */
    char operands[MAX_OPERANDS_PER_LINE][MAX_LINE_LEN]; /* extracted parameters */
    symbol_node *existing_sym; /* pointer for duplicate checks */
    reference_node *old_refs; /* state marker for rollback */
    entry_node *curr_entry; /* iterator for entry validation */
    boolean has_error = false; /* total error flag */
    boolean line_err; /* isolated line error flag */
    boolean line_has_label; /* label flag */
    int line_num = 0; /* current line number */
    int line_length; /* measured length without newline */
    int current_ic; /* saved ic before instruction */
    int current_dc; /* saved dc before directive */
    int i; /* string index */
    int j; /* loop counter for data address update */

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
        printf("error memory allocation failed for system images\n");
        if (state->code_image != NULL) free(state->code_image);
        if (state->data_image != NULL) free(state->data_image);
        state->code_image = NULL;
        state->data_image = NULL;
        return false;
    }

    am_file = fopen(file_name, "r");
    if (am_file == NULL) {
        printf("error opening am file %s\n", file_name);
        free(state->code_image);
        free(state->data_image);
        state->code_image = NULL;
        state->data_image = NULL;
        return false;
    }

    while (fgets(line, sizeof(line), am_file) != NULL) {
        line_num++;
        i = 0;
        line_err = false;

        line_length = strlen(line);
        if (line_length > 0 && line[line_length - 1] == '\n') {
            line_length--;
        }

        if (line_length > MAX_SOURCE_LINE) {
            printf("error in line %d line exceeds maximum length of %d\n", line_num, MAX_SOURCE_LINE);
            has_error = true;
            if (strchr(line, '\n') == NULL && !feof(am_file)) {
                while (fgets(line, sizeof(line), am_file) != NULL && strchr(line, '\n') == NULL) {}
            }
            continue;
        }

        if (is_empty_or_comment(line)) {
            continue;
        }

        line_has_label = has_label(line, label_name);
        if (line_has_label) {
            skip_spaces(line, &i);
            while (line[i] != ' ' && line[i] != '\t' && line[i] != '\0') {
                i++;
            }
            if (line[i] != ' ' && line[i] != '\t') {
                printf("error in line %d label must be followed by whitespace and an operation\n", line_num);
                line_err = true;
                line_has_label = false;
            }
            skip_spaces(line, &i);
        }

        get_first_word(line + i, first_word);

        current_ic = state->ic;
        current_dc = state->dc;
        old_refs = state->refs_head;

        if (strcmp(first_word, ".extern") == 0 || strcmp(first_word, ".entry") == 0) {
            int op_count;

            if (line_has_label) {
                printf("warning in line %d meaningless label before directive\n", line_num);
            }

            while (line[i] != ' ' && line[i] != '\t' && line[i] != '\0') {
                i++;
            }

            if (!extract_operands(line + i, operands, &op_count, MAX_OPERANDS_PER_LINE)) {
                printf("error in line %d invalid operand structure\n", line_num);
                line_err = true;
            } else if (op_count != 1) {
                printf("error in line %d expected exactly 1 operand\n", line_num);
                line_err = true;
            } else if (!is_valid_label_name(operands[0]) || is_reserved_word(operands[0])) {
                printf("error in line %d invalid format or reserved word for label %s\n", line_num, operands[0]);
                line_err = true;
            } else {
                if (strcmp(first_word, ".extern") == 0) {
                    state->has_extern_directive = true;
                    existing_sym = find_symbol(*sym_table, operands[0]);
                    if (existing_sym != NULL && existing_sym->type != SYMBOL_EXTERN) {
                        printf("error in line %d symbol %s is already defined locally\n", line_num, operands[0]);
                        line_err = true;
                    } else if (existing_sym == NULL) {
                        if (add_symbol(sym_table, operands[0], 0, SYMBOL_EXTERN) == NULL) {
                            printf("error in line %d memory allocation failed for symbol\n", line_num);
                            line_err = true;
                        }
                    }
                } else if (strcmp(first_word, ".entry") == 0) {
                    state->has_entry_directive = true;
                    if (!entry_exists(state->entries_head, operands[0])) {
                        if (!add_entry_request(&(state->entries_head), operands[0], line_num)) {
                            printf("error in line %d memory allocation failed for entry request\n", line_num);
                            line_err = true;
                        }
                    }
                }
            }
        }
        else {
            if (line_has_label) {
                if (!is_valid_label_name(label_name) || is_reserved_word(label_name) || find_symbol(*sym_table, label_name) != NULL) {
                    printf("error in line %d invalid format reserved word or duplicate label %s\n", line_num, label_name);
                    line_err = true;
                    line_has_label = false;
                }
            }

            if (!line_err) {
                if (first_word[0] == '.') {
                    if (strcmp(first_word, ".db") == 0 || strcmp(first_word, ".dh") == 0 ||
                        strcmp(first_word, ".dw") == 0 || strcmp(first_word, ".asciz") == 0) {

                        handle_directive(line, &i, state, &line_err, line_num);

                        if (!line_err && line_has_label) {
                            if (add_symbol(sym_table, label_name, current_dc, SYMBOL_DATA) == NULL) {
                                printf("error in line %d memory allocation failed for data symbol\n", line_num);
                                line_err = true;
                            }
                        }
                    } else {
                        printf("error in line %d unknown directive %s\n", line_num, first_word);
                        line_err = true;
                    }
                }
                else {
                    handle_instruction(line, &i, first_word, state, &line_err, line_num);

                    if (!line_err && line_has_label) {
                        if (add_symbol(sym_table, label_name, current_ic, SYMBOL_CODE) == NULL) {
                            printf("error in line %d memory allocation failed for code symbol\n", line_num);
                            line_err = true;
                        }
                    }
                }
            }
        }

        if (line_err) {
            has_error = true;
            state->ic = current_ic;
            state->dc = current_dc;
            while (state->refs_head != old_refs) {
                reference_node *temp = state->refs_head;
                state->refs_head = state->refs_head->next;
                free(temp);
            }
        }
    }

    curr_entry = state->entries_head;
    while (curr_entry != NULL) {
        existing_sym = find_symbol(*sym_table, curr_entry->name);
        if (existing_sym != NULL && existing_sym->type == SYMBOL_EXTERN) {
            printf("error in line %d symbol %s cannot be both entry and extern\n", curr_entry->line_num, curr_entry->name);
            has_error = true;
        }
        curr_entry = curr_entry->next;
    }

    if ((unsigned long)state->ic + (unsigned long)state->dc - 1UL > MAX_ADDRESS) {
        printf("error total memory capacity address exceeded limits\n");
        has_error = true;
    }

    if (!has_error) {
        update_data_symbols(*sym_table, state->ic);
        for (j = 0; j < state->dc; j++) {
            state->data_image[j].address += state->ic;
        }
    }

    fclose(am_file);
    return !has_error;
}

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

static void update_data_symbols(symbol_node *head, int final_ic) {
    symbol_node *curr = head; /* iterator */
    while (curr != NULL) {
        if (curr->type == SYMBOL_DATA) {
            curr->address += final_ic;
        }
        curr = curr->next;
    }
}

static void handle_directive(char line[], int *index, memory_state *state, boolean *line_err, int line_num) {
    char dir_type[MAX_LINE_LEN]; /* directive name */
    char operands[MAX_OPERANDS_PER_LINE][MAX_LINE_LEN]; /* extracted parameters */
    int op_count = 0; /* amount of parameters */
    int k; /* loop counter for parsing */
    int j; /* loop counter for strings */
    int temp_j; /* loop counter for validation */
    int end_idx; /* end index for strings */
    long num_val; /* parsed numeric value */
    unsigned long encoded_val; /* safe unsigned cast for masking */

    get_first_word(line + *index, dir_type);

    while (line[*index] != ' ' && line[*index] != '\t' && line[*index] != '\0') {
        (*index)++;
    }

    if (strcmp(dir_type, ".asciz") == 0) {
        j = *index;

        skip_spaces(line, &j);
        if (line[j] != '"') {
            printf("error in line %d missing opening quote for string\n", line_num);
            *line_err = true;
            return;
        }

        j++;
        end_idx = j;
        while (line[end_idx] != '"' && line[end_idx] != '\0' && line[end_idx] != '\n') {
            end_idx++;
        }

        if (line[end_idx] != '"') {
            printf("error in line %d missing closing quote for string\n", line_num);
            *line_err = true;
            return;
        }

        k = end_idx + 1;
        skip_spaces(line, &k);
        if (line[k] != '\0' && line[k] != '\n') {
            printf("error in line %d extraneous text after string\n", line_num);
            *line_err = true;
            return;
        }

        temp_j = j;
        while (temp_j < end_idx) {
            if (!isprint((unsigned char)line[temp_j])) {
                printf("error in line %d non printable character in string\n", line_num);
                *line_err = true;
                return;
            }
            temp_j++;
        }

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

        return;
    }

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
            if (num_val < MIN_8_BIT || num_val > MAX_8_BIT) {
                printf("error in line %d value out of bounds for 8 bit\n", line_num);
                *line_err = true;
                break;
            } else if (ensure_data_capacity(state, 1)) {
                state->data_image[state->dc].value = (unsigned char)(encoded_val & BYTE_MASK);
                state->data_image[state->dc].address = state->dc;
                state->dc += 1;
            } else {
                printf("error in line %d data memory allocation failed\n", line_num);
                *line_err = true;
                break;
            }
        } else if (strcmp(dir_type, ".dh") == 0) {
            if (num_val < MIN_16_BIT || num_val > MAX_16_BIT) {
                printf("error in line %d value out of bounds for 16 bit\n", line_num);
                *line_err = true;
                break;
            } else if (ensure_data_capacity(state, 2)) {
                state->data_image[state->dc].value = (unsigned char)(encoded_val & BYTE_MASK);
                state->data_image[state->dc].address = state->dc;
                state->data_image[state->dc + 1].value = (unsigned char)((encoded_val >> BYTE_SHIFT_8) & BYTE_MASK);
                state->data_image[state->dc + 1].address = state->dc + 1;
                state->dc += 2;
            } else {
                printf("error in line %d data memory allocation failed\n", line_num);
                *line_err = true;
                break;
            }
        } else if (strcmp(dir_type, ".dw") == 0) {
            if (num_val < MIN_32_BIT || num_val > MAX_32_BIT) {
                printf("error in line %d value out of bounds for 32 bit\n", line_num);
                *line_err = true;
                break;
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
                break;
            }
        }
    }
}

static void handle_instruction(char line[], int *index, char opcode_name[], memory_state *state, boolean *line_err, int line_num) {
    char operands[MAX_INSTRUCTION_OPERANDS][MAX_LINE_LEN]; /* parsed operands */
    int op_count = 0; /* amount of parsed operands */
    int type; /* instruction format r i j */
    int opcode; /* machine opcode */
    int funct; /* machine funct */
    int rs = 0; /* source register */
    int rt = 0; /* target register */
    int rd = 0; /* destination register */
    long immed_val = 0; /* numeric immediate */
    unsigned long word = 0; /* final packed word */
    int code_idx; /* insertion index */

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
        word = ((unsigned long)opcode << OPCODE_SHIFT) | ((unsigned long)rs << RS_SHIFT) | ((unsigned long)rt << RT_SHIFT) | ((unsigned long)rd << RD_SHIFT) | ((unsigned long)funct << FUNCT_SHIFT);
    }
    else if (type == 2) {
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
        word = ((unsigned long)opcode << OPCODE_SHIFT) | ((unsigned long)rs << RS_SHIFT) | ((unsigned long)rt << RT_SHIFT) | ((unsigned long)immed_val & IMMED_MASK);
    }
    else if (type == 3) {
        if (strcmp(opcode_name, "hlt") == 0) {
            if (op_count != 0) {
                printf("error in line %d expected 0 operands\n", line_num);
                *line_err = true; return;
            }
            word = ((unsigned long)opcode << OPCODE_SHIFT);
        } else if (strcmp(opcode_name, "jmp") == 0) {
            if (op_count != 1) {
                printf("error in line %d expected 1 operand\n", line_num);
                *line_err = true; return;
            }
            if (parse_register(operands[0], &rs)) {
                word = ((unsigned long)opcode << OPCODE_SHIFT) | JMP_REG_FLAG | ((unsigned long)rs & REG_J_MASK);
            } else if (is_valid_label_name(operands[0]) && !is_reserved_word(operands[0])) {
                word = ((unsigned long)opcode << OPCODE_SHIFT);
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
            word = ((unsigned long)opcode << OPCODE_SHIFT);
            if (!add_reference(&(state->refs_head), operands[0], state->ic, line_num, REF_J_TYPE)) {
                printf("error in line %d memory allocation failed for reference\n", line_num);
                *line_err = true; return;
            }
        }
    }

    code_idx = (state->ic - INITIAL_IC) / 4;
    if (ensure_code_capacity(state)) {
        state->code_image[code_idx].value = word & 0xFFFFFFFFUL;
        state->code_image[code_idx].address = state->ic;
        state->ic += 4;
    } else {
        printf("error in line %d code memory allocation failed\n", line_num);
        *line_err = true;
    }
}