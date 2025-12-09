CMDS := basic_device_bl
BIN_DIR := $(HOME)/Catena/$(BUILD_TARGET)
LOG_DIR := ./logs

all: $(CMDS)

$(CMDS):
	mkdir -p $(BIN_DIR)/$@
	go build -o $(BIN_DIR)/$@/$@ ./examples/$@

# Build all packages (useful for checking compilation)
build:
	go build ./...

# Run tests for all packages
test:
	go test ./...

# Run tests with verbose output
test-verbose:
	go test -v ./...

# Format all Go files
fmt:
	go fmt ./...

# Run go vet for static analysis
vet:
	go vet ./...

# Tidy up go.mod
tidy:
	go mod tidy

# Clean build artifacts and logs
clean:
	rm -rf $(BIN_DIR)
	rm -rf $(LOG_DIR)

# Run the basic_device example
run-basic-device: basic_device
	CATENA_LOG_DIR=$(LOG_DIR) $(BIN_DIR)/basic_device/basic_device

.PHONY: all clean $(CMDS) build test test-verbose fmt vet tidy run-basic-device