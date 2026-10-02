/*
 * file: pre_assembler.c
 * description: this is the pre assembler stage of the program
 * it reads the source file line by line, finds mcro and mcroend and saves the macro bodies
 * it expands the macro calls into the am file and checks things like line length and names
 * that collide between labels and macros before the real passes start
 */

#include "pre_assembler.h"

/*
 * structure:label_node
 * input: none
 * output: none
 * operation:stores a label that was found outside a macro
 */
typedef struct label_node {
    char name[MAX_LINE_LEN];
    struct label_node *next;
} label_node;

/*
 * function: add_label_to_list
 * input: pointer to label list and label name
 * output: true if successful false if memory allocation fails
 * operation: adds a label to the list
 */
static boolean add_label_to_list(label_node **head, char name[]) {
    label_node *new_node = (label_node *)malloc(sizeof(label_node));
    if (new_node == NULL) {
        return false;
    }
    strcpy(new_node->name, name);
    new_node->next = *head;
    *head = new_node;
    return true;
}

/*
 * function:is_label_in_list
 * input: label list and label name
 * output: true if the label was found false otherwise
 * operation: searches for a label by name
 */
static boolean is_label_in_list(label_node *head, char name[]) {
    label_node *curr = head;
    while (curr != NULL) {
        if (strcmp(curr->name, name) == 0) {
            return true;
        }
        curr = curr->next;
    }
    return false;
}

/*
 * function: free_label_list
 * input: label list
 * output: none
 * operation: frees all labels in the list
 */
static void free_label_list(label_node *head) {
    label_node *curr = head;
    label_node *next_node;
    while (curr != NULL) {
        next_node = curr->next;
        free(curr);
        curr = next_node;
    }
}

/*
 * function: is_valid_label_format
 * input: label name
 * output: true if the label format is valid false otherwise
 * operation: checks the basic rules of a label name
 */
static boolean is_valid_label_format(char label[]) {
    int i= 1; /* first character is checked separately */
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
 * function:check_line_length
 * input: line file and line number
 * output: true if the line is too long false otherwise
 * operation: checks the line length and skips the rest if it is too long
 */
static boolean check_line_length(char line[], FILE *file, int line_num) {
    int len = strlen(line);

    if (len > 0 && line[len - 1] == '\n') {
        len--; /* '\n' is not part of the source line length */
    }

    if (len > MAX_SOURCE_LINE) {
        printf("error in line %d line exceeds maximum length of %d\n", line_num, MAX_SOURCE_LINE);

        /* skip the rest of a line that did not fit in the buffer */
        if (strchr(line, '\n') == NULL && !feof(file)) {
            while (fgets(line, MAX_LINE_LEN, file) != NULL && strchr(line, '\n') == NULL) {
            }
        }
        return true;
    }

    return false;
}

/*
 * function:handle_mcroend
 * input: line line number and macro state flags
 * output: none
 * operation: checks mcroend syntax and ends the current macro definition
 */
static void handle_mcroend(char line[], int line_num, boolean *is_def, boolean *has_err) {
    int i = 0;

    skip_spaces(line, &i);

    /* move past mcroend and check that nothing follows it */
    while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\0') {
        i++;
    }
    skip_spaces(line, &i);

    if (line[i] != '\n' && line[i] != '\0') {
        printf("error in line %d extraneous text after mcroend\n", line_num);
        *has_err = true;
    }
    *is_def = false; /* the macro definition ends even if the line has an error */
}

/*
 * function: has_standalone_token
 * input: line and token to search for
 * output: true if the token was found false otherwise
 * operation: searches for a token outside strings and comment lines
 */
static boolean has_standalone_token(char line[], const char *target) {
    int i = 0;
    int j;
    boolean in_quotes = false; /* true while reading characters inside quotes */
    char token[MAX_LINE_LEN];
    if (line[0] == ';') {
        return false; /* comment lines do not need token checks */
    }
    while (line[i] !='\0' && line[i] != '\n') {
        if (line[i] =='"') {
            in_quotes = !in_quotes;
            i++;
            continue;
        }
        /* tokens inside strings are ignored */
        if (!in_quotes && isalpha((unsigned char)line[i])) {
            j = 0;
            while (line[i] != '\0' &&
                   line[i] != '\n' &&
                   (isalnum((unsigned char)line[i]) || line[i] == '_')) {
                if (j < MAX_LINE_LEN - 1) {
                    token[j] = line[i];
                    j++;
                }
                i++;
            }
            token[j] = '\0';
            if (strcmp(token, target) == 0) {
                return true;
            }
        } else {
            i++;
        }
    }
    return false;
}

/*
 * function: find_macro_identifier_in_line
 * input: line and macro list
 * output: pointer to the macro if its name was found null otherwise
 * operation: searches the line for an existing macro name
 */
static macro_node *find_macro_identifier_in_line(char line[], macro_node *head) {
    int i = 0;
    int j;
    boolean in_quotes = false; /* true while reading characters inside quotes */
    char token[MAX_LINE_LEN];
    macro_node *found;
    if (line[0] == ';') {
        return NULL; /* comment lines do not contain macro calls */
    }
    while (line[i] != '\0' && line[i] != '\n') {
        if (line[i] == '"') {
            in_quotes = !in_quotes;
            i++;
            continue;
        }
        if (!in_quotes && isalpha((unsigned char)line[i])) {
            j = 0;
            while (line[i] != '\0' &&
                   line[i] != '\n' &&
                   (isalnum((unsigned char)line[i]) || line[i] == '_')) {
                if (j < MAX_LINE_LEN - 1) {
                    token[j] = line[i];
                    j++;
                }
                i++;
            }
            token[j] = '\0';
            found = find_macro(head, token);
            if (found != NULL) {
                return found;
            }
        } else {
            i++;
        }
    }
    return NULL;
}

/*
 * function: check_missing_macro_name
 * input: line index and line number
 * output: true if the macro name is missing false otherwise
 * operation: checks if mcro is followed by a name
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
 * input: line index macro name buffer error flag and line number
 * output: none
 * operation: reads the macro name from the line
 */
static void extract_macro_name(char line[], int *i, char macro_name[], boolean *local_err, int line_num) {
    int j = 0; /* current position in the macro name buffer */

    if (!isalpha((unsigned char)line[*i])) {
        printf("error in line %d macro name must start with a letter\n", line_num);
        *local_err=true;
    }

    while (line[*i] != ' ' && line[*i] != '\t' && line[*i] != '\n' && line[*i] != '\0') {
        if (j < MAX_LABEL_LEN - 1) {
            macro_name[j] = line[*i];
            j++;
        } else if (j == MAX_LABEL_LEN - 1) {
            /* report the long name once but keep reading the rest of it */
            printf("error in line %d macro name exceeds maximum length\n", line_num);
            *local_err = true;
            j++;
        }
        (*i)++;
    }

    /* keep '\0' inside the macro name buffer even if the name was too long */
    macro_name[j < MAX_LABEL_LEN ? j : MAX_LABEL_LEN - 1] = '\0';
}

/*
 * function: check_macro_extra_text
 * input: line index and line number
 * output: true if extra text was found false otherwise
 * operation: checks that nothing appears after the macro name
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
 * output: true if an invalid character was found false otherwise
 * operation: checks the characters allowed in a macro name
 */
static boolean validate_macro_characters(char macro_name[], int line_num) {
    int k;
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
 * output: true if the name is reserved false otherwise
 * operation: checks that the macro name is not a reserved word
 */
static boolean check_reserved_macro_name(char macro_name[], int line_num) {
    if (is_reserved_word(macro_name)) {
        printf("error in line %d illegal macro name %s\n", line_num, macro_name);
        return true;
    }
    return false;
}

/*
 * function: check_label_collision
 * input: first word macro list label list error flag line number and memory error flag
 * output: none
 * operation: checks if a label has the same name as a macro and saves valid labels
 */
static void check_label_collision(char first_word[], macro_node *head, label_node **labels_head, boolean *line_err, int line_num, boolean *memory_error) {
    int len = strlen(first_word);
    char potential_label[MAX_LINE_LEN];

    /* a label definition ends with ':' */
    if (len > 0 && first_word[len - 1] == ':') {
        strncpy(potential_label, first_word, len - 1);
        potential_label[len - 1] = '\0';

        if (find_macro(head, potential_label) != NULL) {
            printf("error in line %d label name matches existing macro name\n", line_num);
            *line_err = true;
            return;
        }

        /* labels inside a macro body are not saved in this list */
        if (labels_head != NULL) {
            /* save only valid label names for later macro name checks */
            if (is_valid_label_format(potential_label) && !is_reserved_word(potential_label)) {
                if (!add_label_to_list(labels_head, potential_label)) {
                    printf("error in line %d memory allocation failed for label tracking\n", line_num);
                    *line_err = true;
                    *memory_error = true;
                }
            }
        }
    }
}

/*
 * function: handle_mcro_start
 * input: line line number macro lists label list state flags and memory error flag
 * output: none
 * operation: reads and checks a macro name and adds the macro to the list
 */
static void handle_mcro_start(char line[], int line_num, macro_node **head, macro_node **curr, label_node *labels_head, boolean *is_def, boolean *has_err, boolean *memory_error) {
    char macro_name[MAX_LABEL_LEN];
    int i = 0;
    boolean local_err = false; /* tracks errors found on the current mcro line */

    *is_def = true; /* following lines are treated as macro body lines */

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

    /* continue with name checks only if no earlier error was found */
    if (!local_err && validate_macro_characters(macro_name, line_num)) {
        local_err = true;
    }

    if (!local_err && check_reserved_macro_name(macro_name, line_num)) {
        local_err = true;
    }

    if (!local_err && is_label_in_list(labels_head, macro_name)) {
        printf("error in line %d macro name %s was previously used as a label\n", line_num, macro_name);
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
        *memory_error = true;
        *curr = NULL;
    }
}

/*
 * function: process_regular_line
 * input: first word line macro list label list output file error flag line number and memory error flag
 * output: none
 * operation: expands a macro call or writes the original line to the am file
 */
static void process_regular_line(char first_word[], char line[], macro_node *head, label_node **labels_head, FILE *am_file, boolean *has_err, int line_num, boolean *memory_error) {
    macro_node *found_macro;
    macro_line *m_line; /* current line of the macro being expanded */
    int i = 0;
    boolean line_err = false;

    check_label_collision(first_word, head, labels_head, &line_err, line_num, memory_error);
    if (line_err) {
        *has_err = true;
        return;
    }

    found_macro = find_macro(head, first_word);

    /* a macro call must be the only command on its line */
    if (found_macro == NULL && find_macro_identifier_in_line(line, head) != NULL) {
        printf("error in line %d macro call must appear alone in line\n", line_num);
        *has_err = true;
        return;
    }

    if (found_macro != NULL) {
        skip_spaces(line, &i);

        /* move past the macro name and check for extra text */
        while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\0') {
            i++;
        }

        skip_spaces(line, &i);

        if (line[i] != '\n' && line[i] != '\0') {
            printf("error in line %d extraneous text after macro call\n", line_num);
            *has_err = true;
            return;
        }

        /* write all saved macro lines to the am file */
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
 * input: source file name with .as extension and memory error flag
 * output: true if successful false otherwise
 * operation: reads the source file expands macros and creates the am file
 */
boolean run_pre_assembler(char file_name[], boolean *memory_error) {
    char *am_file_name = NULL;
    char line[MAX_LINE_LEN];
    char first_word[MAX_LINE_LEN];
    FILE *as_file;
    FILE *am_file;
    boolean is_macro_def = false; /* true while reading lines of a macro definition */
    boolean has_error = false;
    macro_node *macro_head = NULL;
    macro_node *current_macro = NULL; /* macro currently being defined */
    label_node *seen_labels_head = NULL;
    int line_number = 0;

    as_file = fopen(file_name, "r");
    if (as_file == NULL) {
        printf("error opening file %s\n", file_name);
        return false;
    }

    am_file_name = replace_extension(file_name, ".am");
    if (am_file_name == NULL) {
        printf("error memory allocation failed for am file name\n");
        fclose(as_file);
        *memory_error = true;
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
                current_macro = NULL; /* no macro is being collected after mcroend */
            }
        } else if (strcmp(first_word, "mcro") == 0) {
            handle_mcro_start(line, line_number, &macro_head, &current_macro, seen_labels_head, &is_macro_def, &has_error, memory_error);
        } else if (is_macro_def) {
            /* mcroend cannot appear as part of another line inside a macro */
            if (has_standalone_token(line, "mcroend")) {
                printf("error in line %d mcroend inside macro definition\n", line_number);
                has_error = true;
            } else if (current_macro != NULL) {
                /* current_macro is null if the mcro line itself was invalid */
                boolean local_line_err = false;
                check_label_collision(first_word, macro_head, NULL, &local_line_err, line_number, memory_error);
                if (local_line_err) {
                    has_error = true;
                }
                if (!add_line_to_macro(current_macro, line)) {
                    printf("error in line %d memory allocation failed for macro line\n", line_number);
                    has_error = true;
                    *memory_error = true;
                }
            }
        } else {
            /* mcroend is invalid when there is no open macro definition */
            if (has_standalone_token(line, "mcroend")) {
                printf("error in line %d invalid mcroend placement\n", line_number);
                has_error = true;
            } else {
                process_regular_line(first_word, line, macro_head, &seen_labels_head, am_file, &has_error, line_number, memory_error);
            }
        }

        if (*memory_error) {
            break;
        }
    }

    free_macros(macro_head);
    free_label_list(seen_labels_head);
    fclose(as_file);
    fclose(am_file);
    free(am_file_name);

    return !has_error;
}