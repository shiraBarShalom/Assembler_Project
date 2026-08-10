#include "pre_assembler.h"

/*
 * function: check_line_length
 * input: line buffer file pointer and line number
 * output: true if line is too long false otherwise
 * operation: checks if line exceeds max length and clears the buffer if it does
 */
static boolean check_line_length(char line[], FILE *file, int line_num) {
    int len = strlen(line); /* length of read line */

    if (len > 0 && line[len - 1] == '\n') {
        len--;
    }

    if (len > MAX_SOURCE_LINE) {
        printf("error in line %d line exceeds maximum length of %d\n", line_num, MAX_SOURCE_LINE);

        if (strchr(line, '\n') == NULL && !feof(file)) {
            while (fgets(line, LINE_BUFFER_SIZE, file) != NULL && strchr(line, '\n') == NULL) {
            }
        }

        return true;
    }

    return false;
}

/*
 * function: handle_mcroend
 * input: line buffer line number and state flags
 * output: none
 * operation: validates mcroend syntax and updates macro state
 */
static void handle_mcroend(char line[], int line_num, boolean *is_def, boolean *has_err) {
    int i = 0; /* loop index */

    skip_spaces(line, &i);

    while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\0') {
        i++;
    }

    skip_spaces(line, &i);

    if (line[i] != '\n' && line[i] != '\0') {
        printf("error in line %d extraneous text after mcroend\n", line_num);
        *has_err = true;
    }

    *is_def = false;
}

/*
 * function: check_missing_macro_name
 * input: line character index and line number
 * output: true if missing false otherwise
 * operation: checks if line ends before macro name
 */
static boolean check_missing_macro_name(char line[], int i, int line_num) {
    if (line[i] == '\n' || line[i] == '\0') {
        printf("error in line %d missing macro name\n", line_num);
        return true;
    }
    return false;
}

/*
 * function: extract_macro_name
 * input: line index pointer macro name buffer and local error flag
 * output: none
 * operation: reads characters into macro name buffer
 */
static void extract_macro_name(char line[], int *i, char macro_name[], boolean *local_err, int line_num) {
    int j = 0; /* copy index */

    if (!isalpha((unsigned char)line[*i])) {
        printf("error in line %d macro name must start with a letter\n", line_num);
        *local_err = true;
    }

    while (line[*i] != ' ' && line[*i] != '\t' && line[*i] != '\n' && line[*i] != '\0') {
        if (j < MAX_LABEL_LEN - 1) {
            macro_name[j] = line[*i];
            j++;
        } else if (j == MAX_LABEL_LEN - 1) {
            printf("error in line %d macro name exceeds maximum length\n", line_num);
            *local_err = true;
            j++;
        }
        (*i)++;
    }

    macro_name[j < MAX_LABEL_LEN ? j : MAX_LABEL_LEN - 1] = '\0';
}

/*
 * function: check_macro_extra_text
 * input: line index and line number
 * output: true if extra text exists false otherwise
 * operation: verifies no text follows macro name
 */
static boolean check_macro_extra_text(char line[], int i, int line_num) {
    skip_spaces(line, &i);
    if (line[i] != '\n' && line[i] != '\0') {
        printf("error in line %d extraneous text after macro name\n", line_num);
        return true;
    }
    return false;
}

/*
 * function: validate_macro_characters
 * input: macro name and line number
 * output: true if invalid characters found false otherwise
 * operation: ensures macro name contains only alphanumeric characters or underscores
 */
static boolean validate_macro_characters(char macro_name[], int line_num) {
    int k; /* validation index */
    for (k = 0; macro_name[k] != '\0'; k++) {
        if (!isalnum((unsigned char)macro_name[k]) && macro_name[k] != '_') {
            printf("error in line %d invalid character in macro name\n", line_num);
            return true;
        }
    }
    return false;
}

/*
 * function: check_reserved_macro_name
 * input: macro name and line number
 * output: true if reserved false otherwise
 * operation: prevents using reserved words as macro names
 */
static boolean check_reserved_macro_name(char macro_name[], int line_num) {
    if (is_reserved_word(macro_name)) {
        printf("error in line %d illegal macro name %s\n", line_num, macro_name);
        return true;
    }
    return false;
}

/*
 * function: handle_mcro_start
 * input: line buffer line number macro lists and state flags
 * output: none
 * operation: extracts macro name validates it and adds to list
 */
static void handle_mcro_start(char line[], int line_num, macro_node **head, macro_node **curr, boolean *is_def, boolean *has_err) {
    char macro_name[MAX_LABEL_LEN]; /* parsed name */
    int i = 0; /* reading index */
    boolean local_err = false; /* local string errors */

    *is_def = true;

    skip_spaces(line, &i);
    while (line[i] != ' ' && line[i] != '\t' && line[i] != '\0') {
        i++;
    }
    skip_spaces(line, &i);

    if (check_missing_macro_name(line, i, line_num)) {
        *has_err = true;
        *curr = NULL;
        return;
    }

    extract_macro_name(line, &i, macro_name, &local_err, line_num);

    if (check_macro_extra_text(line, i, line_num)) {
        local_err = true;
    }

    if (!local_err && validate_macro_characters(macro_name, line_num)) {
        local_err = true;
    }

    if (!local_err && check_reserved_macro_name(macro_name, line_num)) {
        local_err = true;
    }

    if (local_err) {
        *has_err = true;
        *curr = NULL;
        return;
    }

    *curr = add_macro(head, macro_name);

    if (*curr == NULL) {
        printf("error in line %d memory allocation failed for macro\n", line_num);
        *has_err = true;
        *curr = NULL;
    }
}

/*
 * function: process_regular_line
 * input: first word line buffer macro list output file error flag and line num
 * output: none
 * operation: expands macro if found or writes the line as is
 */
static void process_regular_line(char first_word[], char line[], macro_node *head, FILE *am_file, boolean *has_err, int line_num) {
    macro_node *found_macro = find_macro(head, first_word); /* lookup result */
    macro_line *m_line; /* macro contents iterator */
    int i = 0; /* index for trailing text check */

    if (found_macro != NULL) {
        skip_spaces(line, &i);

        while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\0') {
            i++;
        }

        skip_spaces(line, &i);

        if (line[i] != '\n' && line[i] != '\0') {
            printf("error in line %d extraneous text after macro call\n", line_num);
            *has_err = true;
            return;
        }

        m_line = found_macro->lines;

        while (m_line != NULL) {
            fputs(m_line->line, am_file);
            m_line = m_line->next;
        }
    } else {
        fputs(line, am_file);
    }
}

/*
 * function: run_pre_assembler
 * input: original file name with as extension
 * output: true if successful false if there were file errors or macro errors
 * operation: orchestrates the pre assembler process line by line
 */
boolean run_pre_assembler(char file_name[]) {
    char *am_file_name = NULL; /* allocated name */
    char line[LINE_BUFFER_SIZE]; /* line buffer */
    char first_word[MAX_LINE_LEN]; /* first token */
    FILE *as_file; /* input pointer */
    FILE *am_file; /* output pointer */
    boolean is_macro_def = false; /* tracking state */
    boolean has_error = false; /* total pass flag */
    macro_node *macro_head = NULL; /* list start */
    macro_node *current_macro = NULL; /* active parsing node */
    int line_number = 0; /* processed lines counter */

    as_file = fopen(file_name, "r");
    if (as_file == NULL) {
        printf("error opening file %s\n", file_name);
        return false;
    }

    am_file_name = replace_extension(file_name, ".am");
    if (am_file_name == NULL) {
        printf("error memory allocation failed for am file name\n");
        fclose(as_file);
        return false;
    }

    am_file = fopen(am_file_name, "w");
    if (am_file == NULL) {
        printf("error creating file %s\n", am_file_name);
        fclose(as_file);
        free(am_file_name);
        return false;
    }

    while (fgets(line, sizeof(line), as_file) != NULL) {
        line_number++;

        if (check_line_length(line, as_file, line_number)) {
            has_error = true;
            continue;
        }

        get_first_word(line, first_word);

        if (strcmp(first_word, "mcroend") == 0) {
            if (!is_macro_def) {
                printf("error in line %d mcroend without matching mcro\n", line_number);
                has_error = true;
            } else {
                handle_mcroend(line, line_number, &is_macro_def, &has_error);
                current_macro = NULL;
            }
        } else if (strcmp(first_word, "mcro") == 0) {
            handle_mcro_start(line, line_number, &macro_head, &current_macro, &is_macro_def, &has_error);
        } else if (is_macro_def) {
            if (current_macro != NULL) {
                if (!add_line_to_macro(current_macro, line)) {
                    printf("error in line %d memory allocation failed for macro line\n", line_number);
                    has_error = true;
                }
            }
        } else {
            process_regular_line(first_word, line, macro_head, am_file, &has_error, line_number);
        }
    }

    free_macros(macro_head);
    fclose(as_file);
    fclose(am_file);
    free(am_file_name);

    return !has_error;
}