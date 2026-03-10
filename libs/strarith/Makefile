# Makefile for libstrarith.a (Pure C String Arithmetic Library)

CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c11 -fPIC
AR = ar
ARFLAGS = rcs

# Object files
OBJS = str_reverse.o str_validate.o c_str_arith_funcs.o

# Library name
LIB = libstrarith.a

# make strarit lib
strarith: all

# Default target
all: $(LIB)

# Build library
$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built $@ successfully"

# Pattern rule for object files
%.o: %.c strarith.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(LIB)

# List library contents
list: $(LIB)
	ar t $(LIB)

# Show symbols
symbols: $(LIB)
	nm -g $(LIB)

.PHONY: all clean list symbols
