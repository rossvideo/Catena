#!/bin/bash


cd ~/Catena/sdks/cpp/build
ninja
ctest

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
  gcovr --root ~/Catena --filter sdks/cpp -e '(.+/)?build/' -e '(.+/)?tests/' --html=coverage/index.html --html-details --lcov=coverage/coverage.info --xml=coverage/coverage.xml
else
  gcovr --root ~/Catena --filter sdks/cpp -e '(.+/)?build/' -e '(.+/)?tests/' --lcov=coverage/coverage.info --xml=coverage/coverage.xml
fi
