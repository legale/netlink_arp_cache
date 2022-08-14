#name of output file
NAME = netlink_arp

#Linker flags
LIBS  =

#Compiler flags
CFLAGS = -Wall -O2

#Compiler
CC = gcc

#SRC=$(wildcard *.c)
SRC = main.c

all: $(NAME)

$(NAME): $(SRC)
		$(CC) $(CFLAGS) $(LIBS) $^ -o build/$@

clean:
		rm -rf build/*
