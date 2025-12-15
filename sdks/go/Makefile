CMDS := basic_device_bl
BIN_DIR := $(HOME)/Catena/$(BUILD_TARGET)
LOG_DIR := ./logs

all: $(CMDS)

$(CMDS):
	mkdir -p $(BIN_DIR)/$@
	go build -o $(BIN_DIR)/$@/$@ ./examples/$@

# Clean build artifacts and logs
clean:
	rm -rf $(BIN_DIR)
	rm -rf $(LOG_DIR)

# Run the basic_device_bl example
run-basic-device: basic_device_bl
	CATENA_LOG_DIR=$(LOG_DIR) $(BIN_DIR)/basic_device_bl/basic_device_bl

.PHONY: all clean $(CMDS) run-basic-device
