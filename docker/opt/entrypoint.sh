#!/bin/bash

SCRIPT_DIR="$(readlink -f "$(dirname -- "${BASH_SOURCE[0]}")")"
. "${SCRIPT_DIR}/common.sh"

print_logo_anim 2 0.01

prompt "Starting Catena SDK full_service example application"
set -ex

/opt/catena-sdk/full_service --device_model="${DEVICE_MODEL:-/opt/catena-sdk/example_device_models/device.minimal.json}"
