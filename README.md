# Assembler_Project

An assembler written in C that translates assembly source files (`.as`) into output files.

Main language: C

It runs a pre-assembler stage that expands macros (`.am`), then a first and second pass, and writes `.ob`, `.ent` and `.ext` output files.

Build with `make`, then run `./assembler file1.as file2.as ...`.
