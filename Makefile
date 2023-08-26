# Name of output file
NAME = arp
# Build dir
BD = ./build

# Linker flags
LDLIBS +=
LDDIRS += -L$(BD)

# Compiler flags
CFLAGS += -Wall -O2
I += -I/usr/include

# Compiler
CC = gcc
AR = ar

# SRC=$(wildcard *.c)
LIBNAME = nlarpcache
SRC_LIB = libnlarpcache.c syslog.c
SRC_BIN = main.c
OBJ_LIB = $(patsubst %.c, $(BD)/%.o, $(SRC_LIB))
OBJ_BIN = $(patsubst %.c, $(BD)/%.o, $(SRC_BIN))

all: $(NAME)

$(NAME): $(BD)/lib$(LIBNAME).a $(BD)/$(NAME)_shared $(BD)/$(NAME)_static
# Combine additional compilation steps here if needed
# ...

$(BD)/lib$(LIBNAME).a: $(OBJ_LIB)
	$(AR) rcs $@ $^

$(BD)/$(NAME)_shared: $(OBJ_BIN) $(BD)/lib$(LIBNAME).so
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $^ -o $@

$(BD)/$(NAME)_static: $(OBJ_BIN) $(BD)/lib$(LIBNAME).a
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) -Wl,-Bstatic -l$(LIBNAME) -Wl,-Bdynamic $^ -o $@

$(BD)/%.o: %.c
	$(CC) $(CFLAGS) $(I) -c $< -o $@

$(BD)/lib$(LIBNAME).so: $(OBJ_LIB)
	$(CC) $(CFLAGS) $(I) $(LDDIRS) $(LDLIBS) $^ -shared -fPIC -o $@

clean:
	rm -rf $(BD)/*
