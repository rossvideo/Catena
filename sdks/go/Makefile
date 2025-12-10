CMDS := basic_device_bl
BIN_DIR := $(HOME)/Catena/$(BUILD_TARGET)

all: $(CMDS)

$(CMDS):
	mkdir -p $(BIN_DIR)/$@
	go build -o $(BIN_DIR)/$@/$@ ./examples/$@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean $(CMDS)