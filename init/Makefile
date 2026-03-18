include ../config.mk

INIT_SRC = init.c
INIT_BIN = $(BUILD_DIR)init

all: $(INIT_BIN)

#init
$(INIT_BIN): $(INIT_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<
