#settings
CROSS_COMPILE ?=
CC = $(CROSS_COMPILE)gcc
CFLAGS = -static -Wall -Wno-unused-result -Os -s

#ver
DonutOS_Version = 4.2

#out
BUILD_DIR = ../build/
BIN_BUILD_DIR = $(BUILD_DIR)bin
GAMES_BUILD_DIR = $(BUILD_DIR)games