/*
 * file: utils.c
 * description: small helper functions used all over the project
 * it can skip spaces, get the first word of a line, check if a line is empty
 * or a comment, check if a word is a reserved word and build a new file name
 * with a different extension
 */

#include "utils.h"

#define NUM_RESERVED_WORDS 29
#define MAX_REGISTER_NUM 31

/*
 * function: skip_spaces
 * input: line and pointer to index
 * output: none
 * operation: moves the index past spaces and tabs
 */
void skip_spaces(char line[], int *index) {
    while (line[*index] == ' ' || line[*index] == '\t'){
        (*index)++;
    }
}

/*
 * function: get_first_word
 * input: line and buffer for the first word
 * output: none
 * operation: copies thefirst word from the line
 */
void get_first_word(char line[], char word[]){
    int i = 0;
    int j = 0;
    skip_spaces(line, &i);

    while (line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\0') {
        word[j] = line[i];
        i++;
        j++;
    }
    
    word[j]='\0';
}

/*
 * function: is_empty_or_comment
 * input: source line
 * output: true if the line is empty or a comment false otherwise
 * operation: checks if the line should be ignored
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
 * input: word to check
 * output: true if the word is reserved false otherwise
 * operation: checks if the word is an instruction directive or register name
 */
boolean is_reserved_word(char word[]) {
    int i; 
    int j; 
    int reg_num; 
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
        j = 1;
        reg_num = 0;
        
        if (word[j] == '\0') {
            return false;
        }
        
        if (word[j] == '0' && word[j + 1] != '\0') {
            return false;
        }
        
        while (word[j] != '\0') {
            if (!isdigit((unsigned char)word[j])) {
                return false;
            }
            
            reg_num = reg_num * 10 + (word[j] - '0');
            
            if (reg_num > MAX_REGISTER_NUM) {
                return false;
            }
            j++;
        }
        
        return true;
    }
    
    return false;
}

/*
 * function: replace_extension
 * input: file name and new extension
 * output: new file name or null if memory allocation fails
 * operation: creates a new file name with the given extension
 */
char *replace_extension(const char file_name[], const char new_extension[]) {
    const char *last_dot;
    char *new_name;
    size_t base_length;
    size_t total_length;

    if (file_name == NULL || new_extension == NULL) {
        return NULL;
    }

    last_dot = strrchr(file_name, '.');

    if (last_dot == NULL) {
        base_length = strlen(file_name);
    } else {
        base_length = (size_t)(last_dot - file_name);
    }

    total_length = base_length + strlen(new_extension) + 1;

    new_name = (char *)malloc(total_length);
    if (new_name == NULL) {
        return NULL;
    }

    memcpy(new_name, file_name, base_length);
    new_name[base_length] = '\0';
    strcat(new_name, new_extension);

    return new_name;
}