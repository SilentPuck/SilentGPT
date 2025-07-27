

TARGET = silentgpt


SRC_DIR = src
OBJ_DIR = build
INC_DIR = include


SRCS = $(wildcard $(SRC_DIR)/*.c)


OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))


CFLAGS = -Wall -Wextra -O2 -I$(INC_DIR)
CC = gcc


all: $(TARGET)


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lcrypto -lcurl


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)


clean:
	rm -rf $(OBJ_DIR) $(TARGET)
