# Assembler Project

An assembler written in C. I built it as a university project during my Computer Science studies.

## What it does

The program takes one or more assembly source files (`.as`) and processes each one in stages:

- **Pre-assembler:** finds macro definitions (`mcro` ... `mcroend`), expands macro calls and writes the result to an `.am` file.
- **First pass:** reads the `.am` file line by line, parses labels, directives and instructions, builds the code and data images and adds labels to the symbol table. References that can't be resolved yet are saved for later.
- **Second pass:** handles `.entry` requests and fills in the addresses saved during the first pass. Uses of external symbols are recorded.
- **Output:** writes the `.ob` file (code and data image), plus `.ent` and `.ext` files for entry and external symbols.

Errors are reported with the line number, and output files aren't created for a file that has errors.

## Build and run

```
make
./assembler file1.as file2.as
```
