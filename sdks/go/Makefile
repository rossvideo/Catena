CMDS := basic_device_bl
BIN_DIR := $(HOME)/Catena/$(BUILD_TARGET)
LOG_DIR := ./logs

# Proto config
GOBIN       := $(shell go env GOPATH)/bin
PROTO_DIR   := $(abspath ../../smpte/interface/proto)
OUT_DIR     := $(abspath ../../build/go/protos)
MOD_DIR     := $(abspath ../../build/go)
IMPORT_PATH := github.com/rossvideo/catena/build/go/protos
PROTO_FILES := $(wildcard $(PROTO_DIR)/*.proto)
GO_M_OPTS   := $(patsubst %,--go_opt=M%=$(IMPORT_PATH),$(notdir $(PROTO_FILES)))

all: $(CMDS)

$(CMDS):
	mkdir -p $(BIN_DIR)/$@
	go build -o $(BIN_DIR)/$@/$@ ./examples/$@

# Clean build artifacts and logs
clean:
	rm -rf $(BIN_DIR)
	rm -rf $(LOG_DIR)

.PHONY: proto-module proto-tools protos protos-grpc update-imports clean-protos

proto-module:
	mkdir -p "$(MOD_DIR)"
	{ \
		echo "module github.com/rossvideo/catena/build/go"; \
		echo ""; \
		echo "go 1.24"; \
	} > "$(MOD_DIR)/go.mod"

proto-tools:
	@echo "Ensuring protoc plugins..."
	@$(GOBIN)/protoc-gen-go -h >/dev/null 2>&1 || GOBIN=$(GOBIN) go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
	@$(GOBIN)/protoc-gen-go-grpc -h >/dev/null 2>&1 || GOBIN=$(GOBIN) go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest

protos: proto-module proto-tools
	mkdir -p $(OUT_DIR)
	PATH="$(PATH):$(GOBIN)" protoc -I "$(PROTO_DIR)" \
		--go_out="$(OUT_DIR)" \
		--go_opt=paths=source_relative \
		$(GO_M_OPTS) \
		$(PROTO_FILES)

protos-grpc: proto-module proto-tools
	mkdir -p $(OUT_DIR)
	PATH="$(PATH):$(GOBIN)" protoc -I "$(PROTO_DIR)" \
		--go-grpc_out="$(OUT_DIR)" \
		--go-grpc_opt=paths=source_relative \
		--go-grpc_opt=Mservice.proto=$(IMPORT_PATH) \
		"$(PROTO_DIR)/service.proto"

update-imports:
	@echo "Updating imports to $(IMPORT_PATH)"
	grep -RIl 'github.com/rossvideo/catena/sdks/go/protos' ../.. | xargs -r sed -i \
		's#github.com/rossvideo/catena/sdks/go/protos#$(IMPORT_PATH)#g'

clean-protos:
	rm -rf "$(OUT_DIR)"
	