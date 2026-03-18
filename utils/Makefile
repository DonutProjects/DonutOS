include ../config.mk

SRC := $(wildcard *.c)
BIN := $(SRC:.c=)
TARGETS := $(addprefix $(BIN_BUILD_DIR)/,$(BIN))

all: $(TARGETS)

$(BIN_BUILD_DIR)/%: %.c
	@mkdir -p $(BIN_BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<
