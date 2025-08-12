#!/bin/sh

CONNECTION="${CATENA_CONNECTION:-gRPC}"
EXAMPLE="${CATENA_EXAMPLE:-one_of_everything-gRPC}"

EXAMPLE_PATH="$CONNECTION/$EXAMPLE"

if [ -e "$EXAMPLE_PATH" ]; then
    echo "Using example $EXAMPLE_PATH"
else
    echo "Example $EXAMPLE_PATH does not exist, exiting."
    exit 1
fi

OPTIONS=""

# build up command line options based on env vars
OPTIONS="$OPTIONS --port $CATENA_PORT"

if [ -n "$CATENA_CERTS" ]; then
    OPTIONS="$OPTIONS --certs $CATENA_CERTS"
fi

if [ -n "$CATENA_SECURE_COMMS" ]; then
    OPTIONS="$OPTIONS --secure_comms $CATENA_SECURE_COMMS"
fi

if [ -n "$CATENA_CERT_FILE" ]; then
    OPTIONS="$OPTIONS --cert_file $CATENA_CERT_FILE"
fi

if [ -n "$CATENA_KEY_FILE" ]; then
    OPTIONS="$OPTIONS --key_file $CATENA_KEY_FILE"
fi

if [ -n "$CATENA_CA_FILE" ]; then
    OPTIONS="$OPTIONS --ca_file $CATENA_CA_FILE"
fi

if [ -n "$CATENA_PRIVATE_CA" ]; then
    OPTIONS="$OPTIONS --private_ca $CATENA_PRIVATE_CA"
fi

if [ -n "$CATENA_MUTUAL_AUTHC" ]; then
    OPTIONS="$OPTIONS --mutual_authc $CATENA_MUTUAL_AUTHC"
fi

if [ -n "$CATENA_AUTHZ" ]; then
    OPTIONS="$OPTIONS --authz"
fi

STATIC_ROOT="${CATENA_STATIC_ROOT:-$EXAMPLE_PATH/static}"
OPTIONS="$OPTIONS --static_root $STATIC_ROOT"

if [ -n "$CATENA_DEFAULT_MAX_ARRAY_SIZE" ]; then
    OPTIONS="$OPTIONS --default_max_array_size $CATENA_DEFAULT_MAX_ARRAY_SIZE"
fi

if [ -n "$CATENA_DEFAULT_TOTAL_ARRAY_SIZE" ]; then
    OPTIONS="$OPTIONS --default_total_array_size $CATENA_DEFAULT_TOTAL_ARRAY_SIZE"
fi

if [ -n "$CATENA_MAX_CONNECTIONS" ]; then
    OPTIONS="$OPTIONS --max_connections $CATENA_MAX_CONNECTIONS"
fi

echo "Starting Catena with options: $OPTIONS"
exec "$EXAMPLE_PATH/$EXAMPLE" $OPTIONS "$@"
echo "Catena exited with code $?"
exit $?
