CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L

# Build directory for object files
BUILD_DIR=build
# Output directory for binaries
BIN_DIR=bin

BINARIES=smbbroker smbpublish smbsubscribe smbpublish_loop smbpublish_interactive smbkeygen
SECURE_OBJ=$(BUILD_DIR)/smb_secure.o
OBJ_FILES=$(BINARIES:%=$(BUILD_DIR)/%.o) $(SECURE_OBJ)
KEY_FILE=$(BUILD_DIR)/.key
BIN_PATHS=$(BINARIES:%=$(BIN_DIR)/%)
ARGS?=
BROKER_ARGS?=
PUBLISH_ARGS?=
SUBSCRIBE_ARGS?=
PUBLISH_LOOP_ARGS?=
INTERACTIVE_ARGS?=
RUN_GOALS=run-broker run-publish run-subscribe run-publish-loop run-publish-interactive

ifneq ($(filter $(RUN_GOALS),$(MAKECMDGOALS)),)
RUN_ARGS := $(filter-out $(RUN_GOALS),$(MAKECMDGOALS))
.PHONY: $(RUN_ARGS)
$(RUN_ARGS):
	@:
endif

BROKER_RUN_ARGS=$(if $(BROKER_ARGS),$(BROKER_ARGS),$(if $(ARGS),$(ARGS),$(RUN_ARGS)))
PUBLISH_RUN_ARGS=$(if $(PUBLISH_ARGS),$(PUBLISH_ARGS),$(if $(ARGS),$(ARGS),$(RUN_ARGS)))
SUBSCRIBE_RUN_ARGS=$(if $(SUBSCRIBE_ARGS),$(SUBSCRIBE_ARGS),$(if $(ARGS),$(ARGS),$(RUN_ARGS)))
PUBLISH_LOOP_RUN_ARGS=$(if $(PUBLISH_LOOP_ARGS),$(PUBLISH_LOOP_ARGS),$(if $(ARGS),$(ARGS),$(RUN_ARGS)))
INTERACTIVE_RUN_ARGS=$(if $(INTERACTIVE_ARGS),$(INTERACTIVE_ARGS),$(if $(ARGS),$(ARGS),$(RUN_ARGS)))

all: $(BIN_PATHS) $(KEY_FILE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR)/%.o: %.c smb.h smb_secure.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/smbbroker: $(BUILD_DIR)/smbbroker.o $(SECURE_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DIR)/smbpublish: $(BUILD_DIR)/smbpublish.o $(SECURE_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DIR)/smbsubscribe: $(BUILD_DIR)/smbsubscribe.o $(SECURE_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DIR)/smbpublish_loop: $(BUILD_DIR)/smbpublish_loop.o $(SECURE_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DIR)/smbpublish_interactive: $(BUILD_DIR)/smbpublish_interactive.o $(SECURE_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DIR)/smbkeygen: $(BUILD_DIR)/smbkeygen.o | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(KEY_FILE): $(BIN_DIR)/smbkeygen | $(BUILD_DIR)
	@if [ ! -f $@ ]; then ./$(BIN_DIR)/smbkeygen $@; fi

run-broker: $(BIN_DIR)/smbbroker
	./$(BIN_DIR)/smbbroker $(BROKER_RUN_ARGS)

run-publish: $(BIN_DIR)/smbpublish
	./$(BIN_DIR)/smbpublish $(PUBLISH_RUN_ARGS)

run-subscribe: $(BIN_DIR)/smbsubscribe
	./$(BIN_DIR)/smbsubscribe $(SUBSCRIBE_RUN_ARGS)

run-publish-loop: $(BIN_DIR)/smbpublish_loop
	./$(BIN_DIR)/smbpublish_loop $(PUBLISH_LOOP_RUN_ARGS)

run-publish-interactive: $(BIN_DIR)/smbpublish_interactive
	./$(BIN_DIR)/smbpublish_interactive $(INTERACTIVE_RUN_ARGS)

clean:
	rm -f $(BIN_PATHS)
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_DIR)

.PHONY: all clean run-broker run-publish run-subscribe run-publish-loop run-publish-interactive
