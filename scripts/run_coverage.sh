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
  ctest --output-on-failure -V || ctest --rerun-failed --output-on-failure -V
else
  ctest --output-on-failure || ctest --rerun-failed --output-on-failure
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

# clean the coverage directory before generating new reports
rm -rf coverage/*
# Conditionally generate HTML report
if [ "$html_report" = true ]; then
  gcovr --root ~/Catena --filter sdks/cpp -e '(.+/)?build/' -e '(.+/)?tests/' -e '(.+/)?examples/' -e '(.+/)?vdk/' --delete --html=coverage/index.html --html-details --lcov=coverage/coverage.info --xml=coverage/coverage.xml
else
  gcovr --root ~/Catena --filter sdks/cpp -e '(.+/)?build/' -e '(.+/)?tests/' -e '(.+/)?examples/' -e '(.+/)?vdk/' --delete --lcov=coverage/coverage.info --xml=coverage/coverage.xml
fi
