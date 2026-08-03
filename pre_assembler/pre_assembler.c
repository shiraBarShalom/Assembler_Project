#include "pre_assembler.h"

/*
 * function: create_am_name
 * input: original file name
 * output: dynamically allocated am file name or null if invalid extension or allocation failure
 * operation: allocates a buffer and replaces the as extension with am
 */
static char* create_am_name(char file_name[]);

/*
 * function: check_line_length
 * input: line buffer file pointer and line number
 * output: true if line is too long false otherwise
 * operation: checks if line exceeds max length and clears the buffer if it does
 */
static boolean check_line_length(char line[], FILE *file, int line_num);

/*
 * function: handle_mcroend
 * input: line buffer line number and state flags
 * output: none
 * operation: validates mcroend syntax and updates macro state
 */
static void handle_mcroend(char line[], int line_num, boolean *is_def, boolean *has_err);

/*
 * function: handle_mcro_start
 * input: line buffer line number macro lists and state flags
 * output: none
 * operation: extracts macro name validates it and adds to list
 */
static void handle_mcro_start(char line[], int line_num, macro_node **head, macro_node **curr, boolean *is_def, boolean *has_err);

/*
 * function: process_regular_line
 * input: first word line buffer macro list output file line number and error flag
 * output: none
 * operation: expands macro if found and alone on the line reports an error if extra text follows a macro call otherwise writes the line as is
 */
static void process_regular_line(char first_word[], char line[], macro_node *head, FILE *am_file, int line_num, boolean *has_err);

/*
 * function: run_pre_assembler
 * input: original file name with as extension
 * output: true if successful false if there were file errors or macro errors
 * operation: orchestrates the pre assembler process line by line
 */
boolean run_pre_assembler(char file_name[]) {
    char *am_file_name;
    char line[MAX_LINE_LEN];
    char first_word[MAX_LINE_LEN];
    FILE *as_file;
    FILE *am_file;
    boolean is_macro_def = false;
    boolean has_error = false;
    macro_node *macro_head = NULL;
    macro_node *current_macro = NULL;
    int line_number = 0;
    
    as_file = fopen(file_name, "r");
    if (as_file == NULL) {
        printf("error opening file %s\n", file_name);
        return false;
    }
    
    am_file_name = create_am_name(file_name);
    if (am_file_name == NULL) {
        printf("error file name does not have as extension\n");
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
            handle_mcroend(line, line_number, &is_macro_def, &has_error);
            current_macro = NULL;
        } 
        else if (strcmp(first_word, "mcro") == 0) {
            handle_mcro_start(line, line_number, &macro_head, &current_macro, &is_macro_def, &has_error);
        } 
        else if (is_macro_def) {
            if (current_macro != NULL) {
                add_line_to_macro(current_macro, line);
            }
        }
        else {
            process_regular_line(first_word, line, macro_head, am_file, line_number, &has_error);
        }
    }
    
    free_macros(macro_head);
    fclose(as_file);
    fclose(am_file);
    free(am_file_name);

    if (has_error) {
        return false;
    }
    
    return true;
}

/*
 * function: create_am_name
 * input: original file name
 * output: dynamically allocated am file name or null if invalid extension or allocation failure
 * operation: allocates a buffer and replaces the as extension with am
 */
static char* create_am_name(char file_name[]) {
    char *am_name;
    char *dot_ptr;
    size_t len;

    dot_ptr = strrchr(file_name, '.');
    if (dot_ptr == NULL || strcmp(dot_ptr, ".as") != 0) {
        return NULL;
    }

    len = strlen(file_name);
    am_name = (char*)malloc(len + 1);
    if (am_name == NULL) {
        return NULL;
    }

    strcpy(am_name, file_name);
    dot_ptr = strrchr(am_name, '.');
    strcpy(dot_ptr, ".am");

    return am_name;
}

/*
 * function: check_line_length
 * input: line buffer file pointer and line number
 * output: true if line is too long false otherwise
 * operation: checks if line exceeds max length and clears the buffer if it does
 */
static boolean check_line_length(char line[], FILE *file, int line_num) {
    size_t len;
    boolean got_newline;

    len = strlen(line);
    got_newline = false;

    if (len > 0 && line[len - 1] == '\n') {
        len--;
        got_newline = true;
    }

    if (len > MAX_LINE_LEN - 2) {
        printf("error in line %d line exceeds maximum length\n", line_num);
        if (!got_newline) {
            while (fgets(line, MAX_LINE_LEN, file) != NULL && strchr(line, '\n') == NULL) {
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
    int i = 0;
    
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
 * function: handle_mcro_start
 * input: line buffer line number macro lists and state flags
 * output: none
 * operation: extracts macro name validates it and adds to list
 */
static void handle_mcro_start(char line[], int line_num, macro_node **head, macro_node **curr, boolean *is_def, boolean *has_err) {
    char macro_name[MAX_LABEL_LEN];
    int i = 0;
    int j = 0;
    int k;
    int name_start;
    boolean is_valid_name;

    *is_def = true;

    skip_spaces(line, &i);
    while (line[i] != ' ' && line[i] != '\t' && line[i] != '\0') {
        i++;
    }
    skip_spaces(line, &i);

    name_start = i;
    while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\0') {
        if (j < MAX_LABEL_LEN - 1) {
            macro_name[j] = line[i];
            j++;
        }
        i++;
    }
    macro_name[j] = '\0';

    skip_spaces(line, &i);
    if (line[i] != '\n' && line[i] != '\0') {
        printf("error in line %d extraneous text after macro name\n", line_num);
        *has_err = true;
    }

    is_valid_name = true;
    if (!isalpha((unsigned char)macro_name[0])) {
        is_valid_name = false;
    }
    for (k = 1; macro_name[k] != '\0'; k++) {
        if (!isalnum((unsigned char)macro_name[k]) && macro_name[k] != '_') {
            is_valid_name = false;
        }
    }
    if (i - name_start > MAX_LABEL_LEN - 1) {
        is_valid_name = false;
    }

    if (!is_valid_name || is_reserved_word(macro_name) || find_macro(*head, macro_name) != NULL) {
        printf("error in line %d illegal macro name %s\n", line_num, macro_name);
        *has_err = true;
    } else {
        *curr = add_macro(head, macro_name);
    }
}

/*
 * function: process_regular_line
 * input: first word line buffer macro list output file line number and error flag
 * output: none
 * operation: expands macro if found and alone on the line reports an error if extra text follows a macro call otherwise writes the line as is
 */
static void process_regular_line(char first_word[], char line[], macro_node *head, FILE *am_file, int line_num, boolean *has_err) {
    macro_node *found_macro = find_macro(head, first_word);
    macro_line *m_line;
    int i;

    if (found_macro != NULL) {
        i = 0;
        skip_spaces(line, &i);
        i += strlen(first_word);
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