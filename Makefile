CC = gcc
CFLAGS = -Wall -ansi -pedantic -g
EXEC = assembler

# list of all object files in the project
OBJS = assembler.o pre_assembler.o macro.o first_pass.o second_pass.o \
       parser.o symbol_table.o output.o utils.o

# main build rule
all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

# dependencies

assembler.o: assembler.c globals.h symbol_table.h memory_image.h \
             pre_assembler.h first_pass.h second_pass.h output.h utils.h
	$(CC) $(CFLAGS) -c assembler.c

pre_assembler.o: pre_assembler.c pre_assembler.h globals.h macro.h utils.h
	$(CC) $(CFLAGS) -c pre_assembler.c

macro.o: macro.c macro.h globals.h
	$(CC) $(CFLAGS) -c macro.c

first_pass.o: first_pass.c first_pass.h globals.h symbol_table.h \
              memory_image.h parser.h utils.h
	$(CC) $(CFLAGS) -c first_pass.c

second_pass.o: second_pass.c second_pass.h globals.h symbol_table.h memory_image.h
	$(CC) $(CFLAGS) -c second_pass.c

parser.o: parser.c parser.h globals.h utils.h
	$(CC) $(CFLAGS) -c parser.c

symbol_table.o: symbol_table.c symbol_table.h globals.h
	$(CC) $(CFLAGS) -c symbol_table.c

output.o: output.c output.h globals.h symbol_table.h memory_image.h \
          second_pass.h utils.h
	$(CC) $(CFLAGS) -c output.c

utils.o: utils.c utils.h globals.h
	$(CC) $(CFLAGS) -c utils.c

# clean workspace
clean:
	rm -f *.o $(EXEC) *.am *.ob *.ent *.ext