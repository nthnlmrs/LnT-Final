CC = gcc
CFLAGS = -Wall -Iinclude -g $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = -lssl -lcrypto
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32 -lgdi32 -lcrypt32
endif
LDFLAGS += $(shell pkg-config --libs gtk+-3.0)

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

TARGET = $(BIN_DIR)/scanner

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all dirs clean
