/*
 * file: memory_image.h
 * description: gives the structs that hold the code and data images
 * it has code_word and data_byte for one instruction or data byte and memory_state
 * that keeps the whole state of a file while it is being assembled
 */

#ifndef MEMORY_IMAGE_H
#define MEMORY_IMAGE_H

#include "globals.h"
#include "symbol_table.h"

/* maximum address that can fit in 25 bits */
#define MAX_ADDRESS 33554431UL

/* address where the code image starts */
#define INITIAL_IC 100

/* starting size of the code and data arrays */
#define INITIAL_CAPACITY 128

/*
 * structure: code_word
 * operation: stores an instruction address and its encoded value
 */
typedef struct {
    int address;
    unsigned long value;
} code_word;

/*
 * structure: data_byte
 * operation: stores one data byte and its address
 */
typedef struct {
    int address;
    unsigned char value;
} data_byte;

/*
 * structure: memory_state
 * operation: stores the code and data images and the information used during the assembler passes
 */
typedef struct {
    code_word *code_image;
    data_byte *data_image;
    int code_capacity;
    int data_capacity;
    reference_node *refs_head;
    entry_node *entries_head;
    int ic; /* instruction counter */
    int dc; /* data counter */
    boolean has_extern_directive;
    boolean has_entry_directive;
} memory_state;

#endif