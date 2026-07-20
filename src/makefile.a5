#Do not edit the contents of this file.
CC = gcc
CFLAGS = -Werror -Wall -g -std=gnu99
LDFLAGS = -lrt -lpthread
TARGET = banker allocation
OBJFILES = banker.o  allocation.o
all: $(TARGET)
banker: banker.c
	$(CC) $(CFLAGS) -o banker banker.c $(LDFLAGS)

allocation: allocation.c
	$(CC) $(CFLAGS) -o allocation allocation.c

runq1: banker
	./banker 10 5 7 8

runq2: allocation
	./allocation 1048576
clean:
	rm -f $(OBJFILES) $(TARGET)  $(TARGET) *.exe  *~ *.out