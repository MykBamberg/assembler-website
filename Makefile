TARGET := as

CFLAGS := -Wall -Wextra -Wconversion -Werror --std=gnu23
LDFLAGS := -lm

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin

CC := gcc
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
TARGET_PATH := $(BIN_DIR)/$(TARGET)

CFLAGS += -MMD -MP

DEBUG ?= 1
ifeq ($(DEBUG),1)
	CFLAGS += -O0 -Og -g
else
	CFLAGS += -O3
	LDFLAGS += -s
endif

SANITIZE ?= 0
ifeq ($(SANITIZE),1)
	SANITIZER_FLAGS := -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
	CFLAGS += $(SANITIZER_FLAGS)
	LDFLAGS += $(SANITIZER_FLAGS)
endif

all: $(TARGET_PATH)

$(BUILD_DIR):
	@mkdir -p $@

$(BIN_DIR):
	@mkdir -p $@

$(TARGET_PATH): $(OBJS) | $(BIN_DIR)
	$(CC) $^ $(LDFLAGS) -o $@ 

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(BIN_DIR) $(BUILD_DIR)

-include $(DEPS) $(TEST_DEPS)

.PHONY: all clean
