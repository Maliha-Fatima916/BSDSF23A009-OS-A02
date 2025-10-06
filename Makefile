# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin





# Source and target
SRC = $(SRC_DIR)/ls-v1.0.0.c
TARGET = $(BIN_DIR)/ls

# Default target
all: $(TARGET)

# Create binary
$(TARGET): $(SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Create directories
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Install (copy to system bin, optional)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/ls-vl

.PHONY: all clean install
