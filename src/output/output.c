#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "output.h"
#include "utils.h"

/* byte shifting definitions */
#define BYTE_MASK 0xFFUL
#define BYTE_SHIFT_8 8
#define BYTE_SHIFT_16 16
#define BYTE_SHIFT_24 24

/*
 * function: create_object_file
 * input: file name and state struct
 * output: true on success false on fail
 * operation: writes code and data images to ob file
 */
static boolean create_object_file(const char file_name[], memory_state *state) {
    FILE *ob_file; /* file pointer */
    char *output_name; /* dynamic filename */
    int i; /* loop index */
    int address; /* printing address */
    int code_length; /* calculated code size */
    int data_length; /* calculated data size */
    int remaining_data; /* bytes left in data array */
    int data_index; /* current data index */
    unsigned long word; /* full instruction word */

    output_name = replace_extension(file_name, ".ob");
    if (output_name == NULL) {
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

    for (i = 0; i < code_length / 4; i++) {
        word = state->code_image[i].value;
        address = state->code_image[i].address;
        
        fprintf(ob_file, "%04d %02X %02X %02X %02X\n", address,
                (unsigned int)(word & BYTE_MASK),
                (unsigned int)((word >> BYTE_SHIFT_8) & BYTE_MASK),
                (unsigned int)((word >> BYTE_SHIFT_16) & BYTE_MASK),
                (unsigned int)((word >> BYTE_SHIFT_24) & BYTE_MASK));
    }

    remaining_data = data_length;
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
 * input: file name and symbol table
 * output: true on success false on fail
 * operation: writes entry symbols to ent file
 */
static boolean create_entries_file(const char file_name[], symbol_node *symbol_table) {
    FILE *entry_file; /* file pointer */
    char *output_name; /* dynamic filename */
    symbol_node *current; /* symbol iterator */

    output_name = replace_extension(file_name, ".ent");
    if (output_name == NULL) {
        return false;
    }

    entry_file = fopen(output_name, "w");
    if (entry_file == NULL) {
        free(output_name);
        return false;
    }

    current = symbol_table;
    while (current != NULL) {
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
 * input: file name and external usages list
 * output: true on success false on fail
 * operation: writes external usages to ext file
 */
static boolean create_externals_file(const char file_name[], external_usage_node *external_usages) {
    FILE *external_file; /* file pointer */
    char *output_name; /* dynamic filename */
    external_usage_node *current; /* usage list iterator */

    output_name = replace_extension(file_name, ".ext");
    if (output_name == NULL) {
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
 * input: file name
 * output: none
 * operation: removes old output files for the given input file
 */
void remove_output_files(const char file_name[]) {
    char *output_name; /* dynamic filename holder */

    output_name = replace_extension(file_name, ".ob");
    if (output_name != NULL) {
        remove(output_name);
        free(output_name);
    }

    output_name = replace_extension(file_name, ".ent");
    if (output_name != NULL) {
        remove(output_name);
        free(output_name);
    }

    output_name = replace_extension(file_name, ".ext");
    if (output_name != NULL) {
        remove(output_name);
        free(output_name);
    }
}

/*
 * function: create_output_files
 * input: file name memory state symbol table and external usages
 * output: true on success false on failure
 * operation: creates ob ent and ext files as required by directives
 */
boolean create_output_files(const char file_name[], memory_state *state, symbol_node *symbol_table, external_usage_node *external_usages) {
    if (file_name == NULL || state == NULL) {
        return false;
    }

    if (!create_object_file(file_name, state)) {
        remove_output_files(file_name);
        return false;
    }

    if (state->has_entry_directive && !create_entries_file(file_name, symbol_table)) {
        remove_output_files(file_name);
        return false;
    }

    if (state->has_extern_directive && !create_externals_file(file_name, external_usages)) {
        remove_output_files(file_name);
        return false;
    }

    return true;
}