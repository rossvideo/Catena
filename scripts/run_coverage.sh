#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

clean=false
for arg in "$@"; do
  if [[ "$arg" == "-C"  || "$arg" == "--clean"  || "$arg" == "-c" ]]; then
    clean=true
    break
  fi
done

# Check for --html argument
html_report=false
for arg in "$@"; do
  if [ "$arg" == "--html" ]; then
    html_report=true
    break
  fi
done

# Check for -V argument
verbose=false
for arg in "$@"; do
  if [[ "$arg" == "-V"  || "$arg" == "--verbose"  || "$arg" == "-v" ]]; then
    verbose=true
    break
  fi
done

javascript () {
  cd ~/Catena/tools/codegen
  if [ "$clean" = true ]; then
    npm run clean
  fi
  npm install
  npm test
}

cpp () {
  if [ "$clean" = true ]; then
    echo "Cleaning build directory..."
    find ../../ -type f -name '*.gcov' -delete
    rm -rf *
    $CMAKE_COMMAND
    echo "Build directory cleaned."
  fi

  # Set number of parallel jobs for ninja based on available memory 
  ninja_jobs=0
  need_j_value=0
  for arg in "$@"; do
    if [[ $need_j_value -eq 1 ]]; then
      ninja_jobs="$arg"
      break
    fi
    if [[ "$arg" == "-j" ]]; then
      need_j_value=1
    elif [[ "$arg" == -j* ]]; then
      ninja_jobs="${arg:2}"
      break
    fi
  done
  if [ $ninja_jobs -eq 0 ]; then
    ninja_jobs=$(($(nproc) - 1))
    if [ $(free -m | awk '/^Mem:/{print $2}') -le 8192 ]; then
      ninja_jobs=1
    elif [ $(free -m | awk '/^Mem:/{print $2}') -le 16384 ]; then
      ninja_jobs=$(($ninja_jobs - ($ninja_jobs / 4)))
    fi
  fi
  echo "Using $ninja_jobs parallel jobs for ninja build."
  ninja -j $ninja_jobs

  CTEST_FLAGS="--progress --output-on-failure"
  # Run tests with verbose output if -V flag is present
  if [ "$verbose" = true ]; then
    ctest $CTEST_FLAGS -V || ctest $CTEST_FLAGS --rerun-failed -V
  else
    ctest $CTEST_FLAGS || ctest $CTEST_FLAGS --rerun-failed
  fi

  cd ~/Catena/

  #check if coverage directory exists
  if [ ! -d "coverage" ]; then
    mkdir -p coverage
  fi

  # clean the coverage directory before generating new reports
  rm -rf coverage/*
  COVERAGE_FILES="--lcov=coverage/coverage.info --xml=coverage/coverage.xml"
  # Conditionally generate HTML report
  if [ "$html_report" = true ]; then
    gcovr $COVERAGE_FILES --html=coverage/index.html --html-details
  else
    gcovr $COVERAGE_FILES
  fi
}

golang () {
  cd ~/Catena/sdks/go

  if [ "$clean" = true ]; then
    echo "Cleaning Go build cache..."
    go clean -testcache
  fi

  # Download dependencies
  go mod download

  # Run tests with coverage (use count mode for hit counts)
  COVERAGE_FILE="$HOME/Catena/coverage/go_coverage.out"
  mkdir -p ~/Catena/coverage

  if [ "$verbose" = true ]; then
    go test ./pkg/... ./internal/... -v -coverpkg=./pkg/...,./internal -coverprofile="$COVERAGE_FILE" -covermode=count
  else
    go test ./pkg/... ./internal/... -coverpkg=./pkg/...,./internal -coverprofile="$COVERAGE_FILE" -covermode=count
  fi

  # Show coverage summary
  echo ""
  echo "Go Coverage Summary:"
  go tool cover -func="$COVERAGE_FILE" | tail -1

  # Generate LCOV file for Coverage Gutters (VS Code extension)
  echo "Generating LCOV coverage file for VS Code Coverage Gutters..."
  LCOV_FILE="$HOME/Catena/coverage/go_coverage.lcov"
  gcov2lcov -infile="$COVERAGE_FILE" -outfile="$LCOV_FILE"
  echo "LCOV coverage file generated: $LCOV_FILE"

  # Generate HTML report if requested
  if [ "$html_report" = true ]; then
    echo "Generating HTML report with hit counts..."
    gocov convert "$COVERAGE_FILE" | gocov-html > ~/Catena/coverage/go_coverage.html
    echo "Go HTML coverage report (with hit counts): ~/Catena/coverage/go_coverage.html"
  fi
}




# run the main coverage based on build target
cd ~/Catena/${BUILD_TARGET}
echo ~/Catena/${BUILD_TARGET}

if [ "$BUILD_TARGET" == "build/cpp" ]; then
  # run the javascript tests/coverage
  (javascript)
  cpp "$@"
elif [ "$BUILD_TARGET" == "build/go" ]; then
  golang
fi
