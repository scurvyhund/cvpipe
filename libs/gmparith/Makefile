# Makefile for libgmparith.a (GMP Arithmetic Library)

CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c11 -fPIC
AR = ar
ARFLAGS = rcs

# Object files
OBJS = gmp_digits.o gmp_conversion.o gmp_consecutive.o gmp_zones.o

# Library name
LIB = libgmparith.a

# Default target
all: $(LIB)

# Build library
$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^
	@echo "Built $@ successfully"

# Pattern rule for object files
%.o: %.c gmparith.h
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
