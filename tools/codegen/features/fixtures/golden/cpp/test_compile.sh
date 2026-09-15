#!/usr/bin/env bash
#
# Compile-checks the generated C++ golden files against the Catena SDK headers.
# This is a semantic smoke test (-fsyntax-only): it proves the goldens still
# build, catching problems a text diff cannot (e.g. duplicate template
# specializations). It does not produce objects or link.
#
# All paths are discovered relative to the repo / toolchain so the script works
# regardless of checkout location. Override any discovered value via the
# environment: CXX, SDK_INCLUDE, GRPC_INCLUDE, PROTOBUF_INCLUDE.
#
# Usage: ./test_compile.sh            # compile every *.cpp beside this script
#        CXX=clang++ ./test_compile.sh
set -uo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(git -C "$DIR" rev-parse --show-toplevel 2>/dev/null || echo "$DIR/../../../../../..")"
CXX="${CXX:-g++}"

# --- Discover include roots ------------------------------------------------
# SDK headers (Device.h, ParamDescriptor.h, ...). Static location in the repo.
SDK_INCLUDE="${SDK_INCLUDE:-$ROOT/sdks/cpp/common/include}"

# Generated protobuf headers (interface/device.pb.h). Lives under the build
# tree, whose name can vary (build/cpp, build/<target>, ...), so locate it.
if [[ -z "${GRPC_INCLUDE:-}" ]]; then
  pb="$(find "$ROOT" -path '*/grpc_service/interface/device.pb.h' -print -quit 2>/dev/null)"
  GRPC_INCLUDE="${pb%/interface/device.pb.h}"
fi

# Protobuf's own headers (google/protobuf/port_def.inc). Not on the default
# search path here and pkg-config has no entry, so locate the marker header.
if [[ -z "${PROTOBUF_INCLUDE:-}" ]]; then
  inc="$(find /usr/local /usr "$HOME/.local" -path '*/google/protobuf/port_def.inc' -print -quit 2>/dev/null)"
  PROTOBUF_INCLUDE="${inc%/google/protobuf/port_def.inc}"
fi

# --- Sanity check discovered paths -----------------------------------------
fail=0
[[ -f "$SDK_INCLUDE/Device.h" ]] || { echo "ERROR: SDK headers not found (SDK_INCLUDE=$SDK_INCLUDE)"; fail=1; }
[[ -f "$GRPC_INCLUDE/interface/device.pb.h" ]] || { echo "ERROR: generated protobuf headers not found; build the SDK first (GRPC_INCLUDE=$GRPC_INCLUDE)"; fail=1; }
[[ -f "$PROTOBUF_INCLUDE/google/protobuf/port_def.inc" ]] || { echo "ERROR: protobuf headers not found (PROTOBUF_INCLUDE=$PROTOBUF_INCLUDE)"; fail=1; }
[[ $fail -eq 0 ]] || exit 2

# --- Compile every golden --------------------------------------------------
# Only three includes and the C++20 standard are actually required:
#   -std=gnu++20        generated code uses C++20 (coroutines, inline var specializations)
#   -I $SDK_INCLUDE     Catena SDK headers
#   -I $GRPC_INCLUDE    generated protobuf headers (interface/*.pb.h)
#   -isystem $PROTOBUF_INCLUDE  protobuf runtime headers
# The generated .cpp finds its sibling .h automatically (source dir is searched
# for #include "..."), so no -I is needed for this directory.
rc=0
for f in "$DIR"/*.cpp; do
  if "$CXX" -std=gnu++20 -fsyntax-only \
      -I"$SDK_INCLUDE" \
      -I"$GRPC_INCLUDE" \
      -isystem "$PROTOBUF_INCLUDE" \
      "$f" 2>/tmp/test_compile.err; then
    echo "PASS $(basename "$f")"
  else
    echo "FAIL $(basename "$f")"
    head -20 /tmp/test_compile.err
    rc=1
  fi
done
exit $rc