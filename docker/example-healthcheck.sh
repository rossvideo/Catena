#!/bin/sh

if [ $CATENA_CONNECTION = "gRPC" ]; then
    exec grpcurl -plaintext -proto interface/service.proto -import-path interface localhost:$CATENA_PORT catena.CatenaService/GetPopulatedSlots >/dev/null 2>&1 || exit 1
elif [ $CATENA_CONNECTION = "REST" ]; then
    exec wget -q --spider http://localhost:$CATENA_PORT/st2138-api/v1/health || exit 1
else
    echo "Unknown connection type $CATENA_CONNECTION, exiting."
    exit 1
fi