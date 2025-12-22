CC := gcc
CFLAGS := -Wall -Wpedantic -Wextra -Werror

SRC_DIR := src
BIN_DIR := bin
INCLUDE_DIR := include
LIB_DIR := lib

LIBS := -L$(LIB_DIR) -lglfw3 -lm -ldl -lpthread
FRAMEWORKS := -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo

GLAD_OBJECT := third_party/glad.o

.PHONY: all build

all: build

build:
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $(SRC_DIR)/main.c -o $(BIN_DIR)/main $(GLAD_OBJECT) $(LIBS) $(FRAMEWORKS)

prod:
	$(CC) $(CFLAGS) -O3 -I$(INCLUDE_DIR) $(SRC_DIR)/main.c -o $(BIN_DIR)/main $(GLAD_OBJECT) $(LIBS) $(FRAMEWORKS)

run: build build
	./$(BIN_DIR)/main
