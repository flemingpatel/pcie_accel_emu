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

all: $(LIB_STATIC) $(LIB_SHARED)

$(LIB_STATIC): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

$(LIB_SHARED): $(OBJ)
	$(CC) -shared -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(LIB_STATIC) $(LIB_SHARED) $(OBJ)
