include ../config.mk

DOSH_SRC = dosh.c
DOSH_BIN = $(BIN_BUILD_DIR)/dosh

all: $(DOSH_BIN)

#dosh
$(DOSH_BIN): $(DOSH_SRC)
	@mkdir -p $(BIN_BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<
