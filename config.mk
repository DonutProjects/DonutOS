#settings
CROSS_COMPILE ?=
CC            ?= gcc
CFLAGS        ?= -Wall -Os
LDFLAGS       ?= -static -s
LDLIBS        ?=

#ver
DonutOS_Version = 4.3

#out
BUILD_DIR = ../build/
BIN_BUILD_DIR = $(BUILD_DIR)bin
GAMES_BUILD_DIR = $(BUILD_DIR)games