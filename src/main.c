#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "symbol_table.h"
#include "memory_image.h"
#include "pre_assembler.h"
#include "first_pass.h"
#include "second_pass.h"
#include "output.h"

/*
 * function: has_as_extension
 * input: file name string
 * output: true if file ends with .as false otherwise
 * operation: validates that input filename has .as extension
 */
static boolean has_as_extension(const char file_name[]) {
    size_t length; /* file name length */

    if (file_name == NULL) {
        return false;
    }

    length = strlen(file_name);

    if (length >= 3 && strcmp(file_name + length - 3, ".as") == 0) {
        return true;
    }

    return false;
}

/*
 * function: replace_extension
 * input: original file name and new extension string
 * output: dynamically allocated string with new extension or null
 * operation: replaces or appends file extension dynamically
 */
static char *replace_extension(const char file_name[], const char new_extension[]) {
    const char *last_dot; /* pointer to extension */
    char *new_name; /* allocated string */
    size_t base_length; /* original name length without ext */
    size_t total_length; /* total space needed */

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

/*
 * function: initialize_memory_state
 * input: memory state struct pointer
 * output: none
 * operation: sets initial clean values for memory state fields
 */
static void initialize_memory_state(memory_state *state) {
    state->code_image = NULL;
    state->data_image = NULL;
    state->refs_head = NULL;
    state->entries_head = NULL;
    state->code_capacity = 0;
    state->data_capacity = 0;
    state->ic = INITIAL_IC;
    state->dc = 0;
    state->has_extern_directive = false;
    state->has_entry_directive = false;
}

/*
 * function: free_file_resources
 * input: state pointer symbol table pointer and external usages pointer
 * output: none
 * operation: safely frees all allocated memory for a single file run
 */
static void free_file_resources(memory_state *state, symbol_node *symbol_table, external_usage_node *external_usages) {
    free_symbol_table(symbol_table);
    free_references(state->refs_head);
    free_entries(state->entries_head);
    free_external_usages(external_usages);

    free(state->code_image);
    free(state->data_image);

    state->code_image = NULL;
    state->data_image = NULL;
    state->refs_head = NULL;
    state->entries_head = NULL;
}

/*
 * function: main
 * input: argument count and argument vector
 * output: status code zero
 * operation: controls main execution loop for input files through assembler pipeline
 */
int main(int argc, char *argv[]) {
    int i; /* argument loop index */

    if (argc < 2) {
        printf("error no input files provided\n");
        return 0;
    }

    for (i = 1; i < argc; i++) {
        char *am_filename; /* pre assembler output name */
        memory_state state; /* assembler memory structures */
        symbol_node *symbol_table; /* global symbol table */
        external_usage_node *external_usages; /* external records */

        am_filename = NULL;
        symbol_table = NULL;
        external_usages = NULL;

        initialize_memory_state(&state);

        if (!has_as_extension(argv[i])) {
            printf("error file %s must have .as extension\n", argv[i]);
            continue;
        }

        remove_output_files(argv[i]);

        if (!run_pre_assembler(argv[i])) {
            printf("error pre assembler failed for file %s\n", argv[i]);
            continue;
        }

        am_filename = replace_extension(argv[i], ".am");
        if (am_filename == NULL) {
            printf("internal error memory allocation failed\n");
            continue;
        }

        if (!run_first_pass(am_filename, &state, &symbol_table)) {
            printf("error first pass failed for file %s\n", argv[i]);
            free_file_resources(&state, symbol_table, external_usages);
            free(am_filename);
            continue;
        }

        if (!run_second_pass(&state, symbol_table, &external_usages)) {
            printf("error second pass failed for file %s\n", argv[i]);
            free_file_resources(&state, symbol_table, external_usages);
            free(am_filename);
            continue;
        }

        if (!create_output_files(argv[i], &state, symbol_table, external_usages)) {
            printf("error generating output files for file %s\n", argv[i]);
        }

        free_file_resources(&state, symbol_table, external_usages);
        free(am_filename);
    }

    return 0;
}