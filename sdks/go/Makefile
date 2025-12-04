CMDS := basic_device

all: $(CMDS)

$(CMDS):
	mkdir -p $(BUILD_TARGET)
	go build -o $(BUILD_TARGET)/$@ ./examples/$@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean $(CMDS)