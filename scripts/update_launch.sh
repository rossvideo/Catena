#!/bin/bash
LAUNCH_FILE=".vscode/launch.json"
TMP_FILE=".vscode/launch.json.tmp"

# Find all test executables and remove build/cpp/ from each path
find build/cpp/unittests/common -type f -executable -printf '%p\n' > /tmp/common_tests_raw.txt
find build/cpp/unittests/gRPC -type f -executable -printf '%p\n' > /tmp/grpc_tests_raw.txt
find build/cpp/unittests/REST -type f -executable -printf '%p\n' > /tmp/rest_tests_raw.txt

cat /tmp/common_tests_raw.txt /tmp/grpc_tests_raw.txt /tmp/rest_tests_raw.txt | sed 's|build/cpp/||' | awk '{print "        \"" $0 "\","}' > /tmp/all_tests.txt


awk '
BEGIN {in_input=0; in_options=0}
{
  # Detect start of pickTestExecutable input block
  if ($0 ~ /"id": "pickTestExecutable"/) in_input=1
  # Detect start of options array inside the block
  if (in_input && $0 ~ /"options": *\[/) {
    print "      \"options\": ["
    while ((getline line < "/tmp/all_tests.txt") > 0) print line
    print "      ],"
    in_options=1
    next
  }
  # Skip lines inside the original options array
  if (in_options) {
    if ($0 ~ /],/) {
      in_options=0
    }
    next
  }
  print $0
}
' "$LAUNCH_FILE" > "$TMP_FILE"

mv "$TMP_FILE" "$LAUNCH_FILE"
rm /tmp/common_tests_raw.txt /tmp/grpc_tests_raw.txt /tmp/rest_tests_raw.txt /tmp/all_tests.txt

echo "Updated pickTestExecutable options in $LAUNCH_FILE"