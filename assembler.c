/*
 * file: assembler.c
 * description: the entry point of the assembler program
 * it goes over every file given on the command line and checks that the name ends with .as
 * for each file it runs the pre assembler and both passes and then builds the output files
 * if a memory allocation fails it stops the whole program
 */

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
#include "utils.h"

/*
 * function: has_as_extension
 * input: file name string
 * output: true if file ends with .as false otherwise
 * operation: checks if the file name has .as extension
 */
static boolean has_as_extension(const char file_name[]) {
    size_t length; /* length of the file name */

    if (file_name == NULL) {
        return false;
    }

    length = strlen(file_name);

    /* input files must end with .as */
    if (length >= 3 && strcmp(file_name + length - 3, ".as") == 0) {
        return true;
    }

    return false;
}

/*
 * function: initialize_memory_state
 * input: pointer to memory state
 * output: none
 * operation: sets the memory state to its starting values
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
 * input: memory state symbol table and external usages
 * output: none
 * operation: frees the memory used while processing one file
 */
static void free_file_resources(memory_state *state, symbol_node *symbol_table, external_usage_node *external_usages) {
    free_symbol_table(symbol_table);
    free_references(state->refs_head);
    free_entries(state->entries_head);
    free_external_usages(external_usages);

    free(state->code_image);
    free(state->data_image);

    /* clear the pointers after freeing their memory */
    state->code_image = NULL;
    state->data_image = NULL;
    state->refs_head = NULL;
    state->entries_head = NULL;
}

/*
 * function: main
 * input: argument count and argument vector
 * output: zero on normal termination and one on memory allocation failure
 * operation: runs the assembler stages for every input file
 */
int main(int argc, char *argv[]) {
    int i; /* current input file index */

    if (argc < 2) {
        printf("error no input files provided\n");
        return 0;
    }

    /* process every input file separately */
    for (i = 1; i < argc; i++) {
        char *am_filename; /* name of the pre assembler output file */
        memory_state state; /* memory used while processing the file */
        symbol_node *symbol_table; /* symbol table of the current file */
        external_usage_node *external_usages; /* uses of external symbols */
        boolean memory_error = false;

        am_filename = NULL;
        symbol_table = NULL;
        external_usages = NULL;

        initialize_memory_state(&state);

        if (!has_as_extension(argv[i])) {
            printf("error file %s must have .as extension\n", argv[i]);
            continue;
        }

        remove_output_files(argv[i], &memory_error);
        if (memory_error) {
            printf("internal error memory allocation failed\n");
            return 1;
        }

        if (!run_pre_assembler(argv[i], &memory_error)) {
            printf("error pre assembler failed for file %s\n", argv[i]);
            if (memory_error) {
                return 1;
            }
            continue;
        }

        am_filename = replace_extension(argv[i], ".am");
        if (am_filename == NULL) {
            printf("internal error memory allocation failed\n");
            free_file_resources(&state, symbol_table, external_usages);
            return 1;
        }

        /* external usages are still empty before the second pass */
        if (!run_first_pass(am_filename, &state, &symbol_table, &memory_error)) {
            printf("error first pass failed for file %s\n", argv[i]);
            free_file_resources(&state, symbol_table, external_usages);
            free(am_filename);
            if (memory_error) {
                return 1;
            }
            continue;
        }

        if (!run_second_pass(&state, symbol_table, &external_usages, &memory_error)) {
            printf("error second pass failed for file %s\n", argv[i]);
            free_file_resources(&state, symbol_table, external_usages);
            free(am_filename);
            if (memory_error) {
                return 1;
            }
            continue;
        }

        if (!create_output_files(argv[i], &state, symbol_table, external_usages, &memory_error)) {
        if (memory_error) {
            printf("internal error memory allocation failed\n");
            free_file_resources(&state, symbol_table, external_usages);
            free(am_filename);
            return 1;
     } else {
            printf("error generating output files for file %s\n", argv[i]);
        }
    }

        free_file_resources(&state, symbol_table, external_usages);
        free(am_filename);
    }

    return 0;
}