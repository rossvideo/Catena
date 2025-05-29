#!/bin/bash

check_coverage_sync() {
    local build_dir="$1"
    local out_of_sync=false
    
    # Find all .gcda files and check their timestamps against corresponding .gcno files
    while IFS= read -r gcda_file; do
        gcno_file="${gcda_file%.gcda}.gcno"
        if [ -f "$gcno_file" ]; then
            if [ "$gcno_file" -nt "$gcda_file" ]; then
                echo "Coverage data out of sync: $gcda_file is older than $gcno_file"
                out_of_sync=true
            fi
        fi
    done < <(find "$build_dir" -name "*.gcda")
    
    return $([ "$out_of_sync" = true ])
}

cd ~/Catena/build/cpp

# Check if coverage data is out of sync
if check_coverage_sync ~/Catena/build/cpp; then
    echo "Coverage data is out of sync. Cleaning and rebuilding..."
    find ~/Catena/build/cpp -name "*.gcda" -delete    
    ninja clean
else
    echo "Coverage data is in sync. Proceeding with tests..."
fi
ninja
# Check for -V argument
verbose=false
for arg in "$@"; do
  if [[ "$arg" == "-V"  || "$arg" == "--verbose"  || "$arg" == "-v" ]]; then
    verbose=true
    break
  fi
done

# Run tests with verbose output if -V flag is present
if [ "$verbose" = true ]; then
  ctest -V
else
  ctest
fi

cd ~/Catena/

# Check for --html argument
html_report=false
for arg in "$@"; do
  if [ "$arg" == "--html" ]; then
    html_report=true
    break
  fi
done

#check if coverage directory exists
if [ ! -d "coverage" ]; then
  mkdir -p coverage
fi

# Conditionally generate HTML report
if [ "$html_report" = true ]; then
  gcovr --root ~/Catena --filter sdks/cpp -e '(.+/)?build/' -e '(.+/)?tests/' -e '(.+/)?examples/' --html=coverage/index.html --html-details --lcov=coverage/coverage.info --xml=coverage/coverage.xml
else
  gcovr --root ~/Catena --filter sdks/cpp -e '(.+/)?build/' -e '(.+/)?tests/' -e '(.+/)?examples/' --lcov=coverage/coverage.info --xml=coverage/coverage.xml
fi
