CC      := cc
CFLAGS  := -std=c11 -Wall -Wextra -Iinclude -Isrc
LDFLAGS :=

SRCS := \
    src/data_types/t81_bigint.c \
    src/data_types/t81_fraction.c \
    src/data_types/t81_tensor.c \
    src/main.c

OBJS := $(SRCS:.c=.o)

all: t81-foundation

t81-foundation: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS) t81-foundation

.PHONY: all clean
