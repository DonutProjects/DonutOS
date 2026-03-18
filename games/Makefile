include ../config.mk

SRC := $(wildcard *.c)
BIN := $(SRC:.c=)
TARGETS := $(addprefix $(GAMES_BUILD_DIR)/,$(BIN))

all: $(TARGETS)

$(GAMES_BUILD_DIR)/%: %.c
	@mkdir -p $(GAMES_BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<
