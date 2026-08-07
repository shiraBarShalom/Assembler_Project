#include "utils.h"

#define NUM_RESERVED_WORDS 29
#define MAX_REGISTER_NUM 31

/*
 * function: skip_spaces
 * input: char array line and pointer to index
 * output: none
 * operation: skips spaces and tabs
 */
void skip_spaces(char line[], int *index) {
    while (line[*index] == ' ' || line[*index] == '\t') {
        (*index)++;
    }
}

/*
 * function: get_first_word
 * input: char array line and char array word
 * output: none
 * operation: gets first word until space or newline
 */
void get_first_word(char line[], char word[]) {
    int i = 0;
    int j = 0;

    skip_spaces(line, &i);

    while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\0') {
        word[j] = line[i];
        i++;
        j++;
    }
    
    word[j] = '\0';
}

/*
 * function: is_empty_or_comment
 * input: char array line
 * output: true if empty or comment false otherwise
 * operation: checks for comment or empty line
 */
boolean is_empty_or_comment(char line[]) {
    int i = 0; 
    
    if (line[0] == ';') {
        return true;
    }
    
    skip_spaces(line, &i);
    
    if (line[i] == '\0' || line[i] == '\n') {
        return true;
    }
    
    return false;
}

/*
 * function: is_reserved_word
 * input: char array word
 * output: true if reserved false otherwise
 * operation: checks if word is a reserved name
 */
boolean is_reserved_word(char word[]) {
    int i; 
    int j; 
    int reg_num; 
    boolean is_num; 
    const char *instructions[] = {
        "add", "sub", "and", "or", "nor", "move", "mvhi", "mvlo",
        "addi", "subi", "andi", "ori", "nori", "bne", "beq", "blt", "bgt",
        "lb", "sb", "lw", "sw", "lh", "sh",
        "jmp", "la", "call", "hlt",
        "mcro", "mcroend"
    };
    
    for (i = 0; i < NUM_RESERVED_WORDS; i++) {
        if (strcmp(word, instructions[i]) == 0) {
            return true;
        }
    }
    
    if (strcmp(word, ".db") == 0 || strcmp(word, ".dh") == 0 ||
        strcmp(word, ".dw") == 0 || strcmp(word, ".asciz") == 0 ||
        strcmp(word, "asciz") == 0 ||
        strcmp(word, ".entry") == 0 || strcmp(word, ".extern") == 0 ||
        strcmp(word, "db") == 0 || strcmp(word, "dh") == 0 ||
        strcmp(word, "dw") == 0 || strcmp(word, "entry") == 0 ||
        strcmp(word, "extern") == 0) {
        return true;
    }
    
    if (word[0] == '$') {
        is_num = true;
        j = 1;
        if (word[j] == '\0') {
            is_num = false;
        }
        while (word[j] != '\0') {
            if (!isdigit((unsigned char)word[j])) {
                is_num = false;
                break;
            }
            j++;
        }
        if (is_num) {
            reg_num = atoi(word + 1);
            if (reg_num >= 0 && reg_num <= MAX_REGISTER_NUM) {
                return true;
            }
        }
    }
    
    return false;
}