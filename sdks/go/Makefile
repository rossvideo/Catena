CMDS := oneOfEverything hello_world
BIN_DIR := $(HOME)/Catena/$(BUILD_TARGET)
LOG_DIR := ./logs

# Proto config
GOBIN          := $(shell go env GOPATH)/bin
PROTO_DIR      := $(abspath ../../smpte/interface/proto)
GENERATED_DIR  := $(abspath pkg/protos)
RPC_DIR        := $(abspath pkg/protos/rpc)
IMPORT_PATH    := github.com/rossvideo/catena/sdks/go/pkg/protos
RPC_IMPORT_PATH:= github.com/rossvideo/catena/sdks/go/pkg/protos/rpc
PROTO_FILES    := $(wildcard $(PROTO_DIR)/*.proto)

# Message generation: every proto (including the service file's own descriptor)
# resolves to the protos package. protoc-gen-go never imports gRPC, so pkg/protos
# stays gRPC-free regardless of which file a message is declared in.
GO_M_OPTS      := $(patsubst %,--go_opt=M%=$(IMPORT_PATH),$(notdir $(PROTO_FILES)))
# Service generation: message types resolve to protos, but service.proto's stubs
# are emitted into the protos/rpc subpackage so only gRPC users link gRPC.
# protoc-gen-go-grpc emits a file only for protos that declare a service.
GRPC_M_OPTS    := $(patsubst %,--go-grpc_opt=M%=$(IMPORT_PATH),$(notdir $(filter-out $(PROTO_DIR)/service.proto,$(PROTO_FILES)))) --go-grpc_opt=Mservice.proto=$(RPC_IMPORT_PATH)

.PHONY: proto-tools protos clean-protos

all: protos examples

examples: $(CMDS)

$(CMDS):
	mkdir -p $(BIN_DIR)/$@
	go build -o $(BIN_DIR)/$@/$@ ./examples/$@

# Clean build artifacts and logs
clean:
	rm -rf $(BIN_DIR)
	rm -rf $(LOG_DIR)


proto-tools:
	@echo "Ensuring protoc plugins..."
	@which protoc-gen-go >/dev/null 2>&1 || GOBIN=$(GOBIN) go install google.golang.org/protobuf/cmd/protoc-gen-go@latest
	@which protoc-gen-go-grpc >/dev/null 2>&1 || GOBIN=$(GOBIN) go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest

# Generate message types (pkg/protos) and the gRPC service stubs (pkg/protos/rpc)
# together. These are always regenerated as a unit: the service stubs reference
# the message types, so running one without the other would leave the generated
# code inconsistent.
protos: proto-tools
	# These dirs hold only generated output, so wipe and recreate them: renamed or
	# removed protos (and files from an earlier layout) leave no stale .pb.go behind,
	# since protoc only writes the files it currently generates.
	rm -rf $(GENERATED_DIR)
	mkdir -p $(GENERATED_DIR) $(RPC_DIR)
	# Messages and all file descriptors -> pkg/protos
	PATH="$(PATH):$(GOBIN)" protoc -I "$(PROTO_DIR)" \
		--go_out="$(GENERATED_DIR)" \
		--go_opt=paths=source_relative \
		$(GO_M_OPTS) \
		$(PROTO_FILES)
	# gRPC service stubs only -> pkg/protos/rpc
	PATH="$(PATH):$(GOBIN)" protoc -I "$(PROTO_DIR)" \
		--go-grpc_out="$(RPC_DIR)" \
		--go-grpc_opt=paths=source_relative \
		$(GRPC_M_OPTS) \
		$(PROTO_FILES)

clean-protos:
	rm -rf "$(GENERATED_DIR)"

# Test targets
.PHONY: test race coverage coverage-html coverage-lcov coverage-cobertura clean-coverage

# Exclude generated protos from tests + coverage accounting
PROTO_PKG := github.com/rossvideo/catena/sdks/go/pkg/protos
TRANSPORTTEST_PKG := github.com/rossvideo/catena/sdks/go/pkg/transports/internal/transporttest
TEST_PACKAGES := $(shell go list ./pkg/... | grep -vE '^($(PROTO_PKG)|$(TRANSPORTTEST_PKG))($$|/)')

COVERAGE_DIR ?= $(HOME)/Catena/coverage
COVERAGE_OUT := $(COVERAGE_DIR)/go_coverage.out
VERBOSE ?= 0

test:
	go test -v ./...

RACE_PACKAGES ?= $(TEST_PACKAGES)
RACE_COUNT ?= 10

GOTESTSUM = gotestsum \
	--format $(if $(filter 1,$(VERBOSE)),standard-verbose,pkgname) \
	$(GOTESTSUM_OPTS) --

race: protos
	$(GOTESTSUM) \
		-race \
		-count=$(RACE_COUNT) \
		$(if $(filter 1,$(VERBOSE)),-v) \
		$(RACE_PACKAGES)

coverage: | $(COVERAGE_DIR) protos
	$(GOTESTSUM) \
		-coverprofile=$(COVERAGE_OUT) \
		-covermode=count \
		$(if $(filter 1,$(VERBOSE)),-v) \
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

coverage-cobertura: coverage
	@echo "Generating Cobertura coverage file for CI/CD pipelines..."
	gocover-cobertura < $(COVERAGE_OUT) > $(COVERAGE_DIR)/go_coverage.xml
	@echo "Cobertura coverage report generated: $(COVERAGE_DIR)/go_coverage.xml"

clean-coverage:
	@echo "Cleaning Go build cache and coverage files..."
	go clean -testcache
	rm -f $(COVERAGE_OUT)
	rm -f $(COVERAGE_DIR)/go_coverage.html
	rm -f $(COVERAGE_DIR)/go_coverage.lcov
	rm -f $(COVERAGE_DIR)/go_coverage.xml
	@echo "Go build cache cleaned."

$(COVERAGE_DIR):
	mkdir -p $(COVERAGE_DIR)
