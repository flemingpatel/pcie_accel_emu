# Makefile for building the user-space library in src/lib

CC := gcc
CFLAGS := -Wall -Werror -O2 -fPIC
AR := ar
ARFLAGS := rcs

INCLUDES := -I../../include

LIB_STATIC := libvai.a
LIB_SHARED := libvai.so

# List your library source files here
SRC := libvai.c
OBJ := $(SRC:.c=.o)

# Example application
EXAMPLE := example
EXAMPLE_SRC := example.c
EXAMPLE_OBJ := $(EXAMPLE_SRC:.c=.o)

#all: $(LIB_STATIC) $(LIB_SHARED) $(EXAMPLE)
all: $(LIB_SHARED) $(EXAMPLE)

$(LIB_STATIC): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

$(LIB_SHARED): $(OBJ)
	$(CC) -shared -o $@ $^

$(EXAMPLE): $(EXAMPLE_OBJ) $(LIB_SHARED)
	$(CC) -o $@ $< -L. -lvai $(CFLAGS) $(INCLUDES)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(LIB_STATIC) $(LIB_SHARED) $(OBJ) $(EXAMPLE) $(EXAMPLE_OBJ)
