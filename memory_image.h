#ifndef MEMORY_IMAGE_H
#define MEMORY_IMAGE_H

#include "globals.h"
#include "symbol_table.h"

/* Highest legal memory address in a 25-bit address space. */
#define MAX_ADDRESS 33554431UL

/* Initial address of the code image. */
#define INITIAL_IC 100

/* Initial capacity of the dynamically allocated memory images. */
#define INITIAL_CAPACITY 128

/*
 * Type: memory_state
 * Purpose:
 * Stores the code image, data image, counters and the lists
 * required during the assembler passes.
 */
typedef struct {
    unsigned long *code_image;
    unsigned char *data_image;
    int code_capacity;
    int data_capacity;
    reference_node *refs_head;
    entry_node *entries_head;
    int ic; /* Instruction counter. */
    int dc; /* Data counter. */
} memory_state;

#endif