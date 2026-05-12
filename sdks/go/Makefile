CMDS_PAIRED := getAsset getDevice getSetValue oneOfEverything
CMDS_REST_ONLY := executeCommand
BIN_DIR := $(HOME)/Catena/$(BUILD_TARGET)
LOG_DIR := ./logs

# Proto config
GOBIN         := $(shell go env GOPATH)/bin
PROTO_DIR     := $(abspath ../../smpte/interface/proto)
GENERATED_DIR := $(abspath pkg/protos)
IMPORT_PATH   := github.com/rossvideo/catena/sdks/go/pkg/protos
PROTO_FILES   := $(wildcard $(PROTO_DIR)/*.proto)
GO_M_OPTS     := $(patsubst %,--go_opt=M%=$(IMPORT_PATH),$(notdir $(PROTO_FILES)))
GO_GRPC_M_OPTS:= $(patsubst %,--go-grpc_opt=M%=$(IMPORT_PATH),$(notdir $(PROTO_FILES)))

.PHONY: proto-tools protos protos-grpc protos-all clean-protos

all: protos-all paired-examples rest-only-examples

paired-examples: $(foreach cmd,$(CMDS_PAIRED),$(cmd)_REST $(cmd)_GRPC)

$(foreach cmd,$(CMDS_PAIRED),$(cmd)_REST): %_REST:
	mkdir -p $(BIN_DIR)/$*_REST
	go build -o $(BIN_DIR)/$*_REST/$*_REST ./examples/$*/rest

$(foreach cmd,$(CMDS_PAIRED),$(cmd)_GRPC): %_GRPC:
	mkdir -p $(BIN_DIR)/$*_GRPC
	go build -o $(BIN_DIR)/$*_GRPC/$*_GRPC ./examples/$*/grpc

rest-only-examples: $(foreach cmd,$(CMDS_REST_ONLY),$(cmd)_REST)

$(foreach cmd,$(CMDS_REST_ONLY),$(cmd)_REST): %_REST:
	mkdir -p $(BIN_DIR)/$*_REST
	go build -o $(BIN_DIR)/$*_REST/$*_REST ./examples/$*/rest

# Clean build artifacts and logs
clean:
	rm -rf $(BIN_DIR)
	rm -rf $(LOG_DIR)


protos-all: protos protos-grpc

proto-tools:
	@echo "Ensuring protoc plugins..."
	@which protoc-gen-go >/dev/null 2>&1 || GOBIN=$(GOBIN) go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
	@which protoc-gen-go-grpc >/dev/null 2>&1 || GOBIN=$(GOBIN) go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest

protos: proto-tools
	mkdir -p $(GENERATED_DIR)
	PATH="$(PATH):$(GOBIN)" protoc -I "$(PROTO_DIR)" \
		--go_out="$(GENERATED_DIR)" \
		--go_opt=paths=source_relative \
		$(GO_M_OPTS) \
		$(PROTO_FILES)

protos-grpc: proto-tools
	mkdir -p $(GENERATED_DIR)
	PATH="$(PATH):$(GOBIN)" protoc -I "$(PROTO_DIR)" \
		--go-grpc_out="$(GENERATED_DIR)" \
		--go-grpc_opt=paths=source_relative \
		$(GO_GRPC_M_OPTS) \
		"$(PROTO_DIR)/service.proto"

clean-protos:
	rm -rf "$(GENERATED_DIR)"

# Test coverage targets
.PHONY: test coverage coverage-html coverage-lcov clean-coverage

# Exclude generated protos from tests + coverage accounting
PROTO_PKG := github.com/rossvideo/catena/sdks/go/pkg/protos
TEST_PACKAGES := $(shell go list ./pkg/... | grep -vE '^$(PROTO_PKG)($$|/)')

# coverpkg takes a comma-separated list
COVER_PACKAGES := $(shell go list ./pkg/... | grep -vE '^$(PROTO_PKG)($$|/)' | paste -sd, -)

COVERAGE_DIR ?= $(HOME)/Catena/coverage
COVERAGE_OUT := $(COVERAGE_DIR)/go_coverage.out
VERBOSE ?= 0

test:
	go test -v ./...

coverage: | $(COVERAGE_DIR) protos-all
	go test \
		-coverprofile=$(COVERAGE_OUT) \
		-covermode=count \
		$(if $(filter 1,$(VERBOSE)),-v) \
		-coverpkg=$(COVER_PACKAGES) \
		$(TEST_PACKAGES)
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
