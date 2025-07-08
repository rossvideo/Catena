#!/bin/bash

cd ~/Catena/${BUILD_TARGET}
echo ~/Catena/${BUILD_TARGET}
clean=false
for arg in "$@"; do
  if [[ "$arg" == "-C"  || "$arg" == "--clean"  || "$arg" == "-c" ]]; then
    clean=true
    break
  fi
done

if [ "$clean" = true ]; then
  echo "Cleaning build directory..."
  ninja clean
  echo "Build directory cleaned."
fi

ninja -j20
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
