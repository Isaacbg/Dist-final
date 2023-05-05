CC = gcc
CFLAGS = -lrt
OBJS = servidor 
CLAVES_PATH = .
BIN_FILES = servidor lines sll

all: $(OBJS)
	
servidor: servidor.c sll.o lines.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(BIN_FILES) *.o *.so *.out 

re: clean all

.PHONY: all servidor sll lines