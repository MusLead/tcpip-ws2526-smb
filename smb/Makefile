CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L

# Output directory for binaries
BIN_DIR=bin

BINARIES=smbbroker smbpublish smbsubscribe smbpublish_loop smbpublish_interactive
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

all: $(BIN_PATHS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/smbbroker: smbbroker.c smb.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ smbbroker.c

$(BIN_DIR)/smbpublish: smbpublish.c smb.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ smbpublish.c

$(BIN_DIR)/smbsubscribe: smbsubscribe.c smb.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ smbsubscribe.c

$(BIN_DIR)/smbpublish_loop: smbpublish_loop.c smb.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ smbpublish_loop.c

$(BIN_DIR)/smbpublish_interactive: smbpublish_interactive.c smb.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ smbpublish_interactive.c

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
	rm -rf $(BIN_DIR)

.PHONY: all clean run-broker run-publish run-subscribe run-publish-loop run-publish-interactive
