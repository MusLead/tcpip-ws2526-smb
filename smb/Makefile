CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L

# Build directory for object files
BUILD_DIR=build

TARGETS=smbbroker smbpublish smbsubscribe
OBJ_FILES=$(BUILD_DIR)/smbbroker.o $(BUILD_DIR)/smbpublish.o $(BUILD_DIR)/smbsubscribe.o

all: $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c smb.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

smbbroker: $(BUILD_DIR)/smbbroker.o
	$(CC) $(CFLAGS) -o smbbroker $<

smbpublish: $(BUILD_DIR)/smbpublish.o
	$(CC) $(CFLAGS) -o smbpublish $<

smbsubscribe: $(BUILD_DIR)/smbsubscribe.o
	$(CC) $(CFLAGS) -o smbsubscribe $<

clean:
	rm -f smbbroker smbpublish smbsubscribe
	rm -rf $(BUILD_DIR)

.PHONY: all clean