/*
 * file: output.c
 * description: builds the final output files of the assembler
 * it writes the code and data image to the ob file, the entry symbols to the ent file
 * and the external symbol uses to the ext file, it also removes old output files
 * before a new run and cleans up if one of the files could not be created
 */

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include "output.h" 
#include "utils.h" 

 
/* values used to split a word into bytes */ 
#define BYTE_MASK 0xFFUL 
#define BYTE_SHIFT_8 8 
#define BYTE_SHIFT_16 16 
#define BYTE_SHIFT_24 24 
 
/* 
 * function: create_object_file 
 * input: file name memory state and memory error flag 
 * output: true if successful false otherwise 
 * operation: writes the code and data images to the ob file 
 */ 
static boolean create_object_file(const char file_name[], memory_state *state, boolean *memory_error) { 
    FILE *ob_file; 
    char *output_name; 
    int i; 
    int address; 
    int code_length; 
    int data_length; 
    int remaining_data; /* number of data bytes left to print */ 
    int data_index; /* current position in the data image */ 
    unsigned long word; /* current code word */ 
 
    output_name = replace_extension(file_name, ".ob"); 
    if (output_name == NULL) { 
        *memory_error = true; 
        return false; 
    } 
 
    ob_file = fopen(output_name, "w"); 
    if (ob_file == NULL) { 
        free(output_name); 
        return false; 
    } 
 
    code_length = state->ic - INITIAL_IC; 
    data_length = state->dc; 
    data_index = 0; 
 
    fprintf(ob_file, "%d %d\n", code_length, data_length); 
 
    /* each instruction is stored as one 4 byte word */ 
    for (i = 0; i < code_length / 4; i++) { 
        word = state->code_image[i].value; 
        address = state->code_image[i].address; 
 
        /* print the bytes from least significant to most significant */ 
        fprintf(ob_file, "%04d %02X %02X %02X %02X\n", address, 
                (unsigned int)(word & BYTE_MASK), 
                (unsigned int)((word >> BYTE_SHIFT_8) & BYTE_MASK), 
                (unsigned int)((word >> BYTE_SHIFT_16) & BYTE_MASK), 
                (unsigned int)((word >> BYTE_SHIFT_24) & BYTE_MASK)); 
    } 
 
    remaining_data = data_length; 
    /* print data in rows of up to 4 bytes */ 
    while (remaining_data > 0) { 
        fprintf(ob_file, "%04d", state->data_image[data_index].address); 
        for (i = 0; i < 4 && remaining_data > 0; i++) { 
            fprintf(ob_file, " %02X", (unsigned int)state->data_image[data_index].value); 
            data_index++; 
            remaining_data--; 
        } 
        fprintf(ob_file, "\n"); 
    } 
 
    if (fclose(ob_file) != 0) { 
        free(output_name); 
        return false; 
    } 
 
    free(output_name); 
    return true; 
} 
 
/* 
 * function: create_entries_file 
 * input: file name symbol table and memory error flag 
 * output: true if successful false otherwise 
 * operation: writes entry symbols to the ent file 
 */ 
static boolean create_entries_file(const char file_name[], symbol_node *symbol_table, boolean *memory_error) { 
    FILE *entry_file; 
    char *output_name; 
    symbol_node *current; /* current symbol in the table */ 
 
    output_name = replace_extension(file_name, ".ent"); 
    if (output_name == NULL) { 
        *memory_error = true; 
        return false; 
    } 
 
    entry_file = fopen(output_name, "w"); 
    if (entry_file == NULL) { 
        free(output_name); 
        return false; 
    } 
 
    current = symbol_table; 
    while (current != NULL) { 
        /* only symbols marked as entry are written to the file */ 
        if (current->is_entry) { 
            fprintf(entry_file, "%s %04d\n", current->name, current->address); 
        } 
        current = current->next; 
    } 
 
    if (fclose(entry_file) != 0) { 
        free(output_name); 
        return false; 
    } 
 
    free(output_name); 
    return true; 
} 
 
/* 
 * function: create_externals_file 
 * input: file name external usages list and memory error flag 
 * output: true if successful false otherwise 
 * operation: writes external symbol usages to the ext file 
 */ 
static boolean create_externals_file(const char file_name[], external_usage_node *external_usages, boolean *memory_error) { 
    FILE *external_file; 
    char *output_name; 
    external_usage_node *current; /* current external usage in the list */ 
 
    output_name = replace_extension(file_name, ".ext"); 
    if (output_name == NULL) { 
        *memory_error = true; 
        return false; 
    } 
 
    external_file = fopen(output_name, "w"); 
    if (external_file == NULL) { 
        free(output_name); 
        return false; 
    } 
 
    current = external_usages; 
    while (current != NULL) { 
        fprintf(external_file, "%s %04d\n", current->name, current->instruction_address); 
        current = current->next; 
    } 
 
    if (fclose(external_file) != 0) { 
        free(output_name); 
        return false; 
    } 
 
    free(output_name); 
    return true; 
} 
 
/* 
 * function: remove_output_files 
 * input: file name and memory error flag 
 * output: none 
 * operation: removes old output files for the input file 
 */ 
void remove_output_files(const char file_name[], boolean *memory_error) { 
    char *output_name; /* name of the file to remove */ 
 
    output_name = replace_extension(file_name, ".ob"); 
    if (output_name != NULL) { 
        remove(output_name); 
        free(output_name); 
    } else { 
        *memory_error = true; 
    } 
 
    output_name = replace_extension(file_name, ".ent"); 
    if (output_name != NULL) { 
        remove(output_name); 
        free(output_name); 
    } else { 
        *memory_error = true; 
    } 
 
    output_name = replace_extension(file_name, ".ext"); 
    if (output_name != NULL) { 
        remove(output_name); 
        free(output_name); 
    } else { 
        *memory_error = true; 
    } 
} 
 
/* 
 * function: create_output_files 
 * input: file name memory state symbol table external usages list and memory error flag 
 * output: true if successful false otherwise 
 * operation: creates the output files needed for the input file 
 */ 
boolean create_output_files(const char file_name[], memory_state *state, symbol_node *symbol_table, external_usage_node *external_usages, boolean *memory_error) { 
    if (file_name == NULL || state == NULL) { 
        return false; 
    } 
 
    if (!create_object_file(file_name, state, memory_error)) { 
        remove_output_files(file_name, memory_error); 
        return false; 
    } 
 
    /* create an ent file only if an entry directive was used */ 
    if (state->has_entry_directive && !create_entries_file(file_name, symbol_table, memory_error)) { 
        remove_output_files(file_name, memory_error); 
        return false; 
    } 
 
    /* create an ext file only if an extern directive was used */ 
    if (state->has_extern_directive && !create_externals_file(file_name, external_usages, memory_error)) { 
        remove_output_files(file_name, memory_error); 
        return false; 
    } 
 
    return true; 
}