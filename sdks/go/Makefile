CMDS_REST := executeCommand_REST getAsset_REST getDevice_REST getSetValue_REST
CMDS_GRPC := getDevice_GRPC getSetValue_GRPC
CMDS := $(CMDS_REST) $(CMDS_GRPC)
BIN_DIR := $(HOME)/Catena/$(BUILD_TARGET)
LOG_DIR := ./logs

# Proto config
GOBIN         := $(shell go env GOPATH)/bin
PROTO_DIR     := $(abspath ../../smpte/interface/proto)
OUT_DIR       := $(abspath pkg/protos)
IMPORT_PATH   := github.com/rossvideo/catena/pkg/protos
PROTO_FILES   := $(wildcard $(PROTO_DIR)/*.proto)
GO_M_OPTS     := $(patsubst %,--go_opt=M%=$(IMPORT_PATH),$(notdir $(PROTO_FILES)))
GO_GRPC_M_OPTS:= $(patsubst %,--go-grpc_opt=M%=$(IMPORT_PATH),$(notdir $(PROTO_FILES)))

.PHONY: proto-module proto-tools protos protos-grpc protos-all clean-protos

all: protos-all $(CMDS)

$(CMDS):
	mkdir -p $(BIN_DIR)/$@
	go build -o $(BIN_DIR)/$@/$@ ./examples/$@

# Clean build artifacts and logs
clean:
	rm -rf $(BIN_DIR)
	rm -rf $(LOG_DIR)


protos-all: protos protos-grpc

proto-tools:
	@echo "Ensuring protoc plugins..."
	@$(GOBIN)/protoc-gen-go -h >/dev/null 2>&1 || GOBIN=$(GOBIN) go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
	@$(GOBIN)/protoc-gen-go-grpc -h >/dev/null 2>&1 || GOBIN=$(GOBIN) go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest

protos: proto-tools
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
		$(GO_GRPC_M_OPTS) \
		"$(PROTO_DIR)/service.proto"

clean-protos:
	rm -rf "$(OUT_DIR)"

# Test coverage targets
.PHONY: test coverage coverage-html coverage-lcov clean-coverage

# Exclude generated protos from coverage
TEST_PACKAGES := $(shell go list ./pkg/... | grep -v "github.com/rossvideo/catena/sdks/go/pkg/protos")
COVERAGE_DIR := $(HOME)/Catena/coverage
COVERAGE_OUT := $(COVERAGE_DIR)/go_coverage.out
VERBOSE ?= 0

test:
	go test -v ./...

coverage: | $(COVERAGE_DIR) protos-all
	go test -coverprofile=$(COVERAGE_OUT) -covermode=count $(TEST_PACKAGES) $(if $(filter 1,$(VERBOSE)),-v)
	@echo ""
	@echo "Coverage Summary:"
	@go tool cover -func=$(COVERAGE_OUT) | tail -1

coverage-html: coverage
	@echo "Generating HTML report with hit counts..."
	gocov convert $(COVERAGE_OUT) | gocov-html > $(COVERAGE_DIR)/go_coverage.html
	@echo "Go HTML coverage report (with hit counts): $(COVERAGE_DIR)/go_coverage.html"

coverage-lcov: coverage
	@echo "Generating LCOV coverage file for VS Code Coverage Gutters..."
	gcov2lcov -infile=$(COVERAGE_OUT) -outfile=$(COVERAGE_DIR)/go_coverage.lcov
	@echo "LCOV coverage report generated: $(COVERAGE_DIR)/go_coverage.lcov"
	@echo "You can now enable Coverage Gutters in VS Code to see coverage highlights"

clean-coverage:
	@echo "Cleaning Go build cache and coverage files..."
	go clean -testcache
	rm -f $(COVERAGE_OUT)
	@echo "Go build cache cleaned."

$(COVERAGE_DIR):
	mkdir -p $(COVERAGE_DIR)
