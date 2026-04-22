if [ "$BUILD_TARGET" == "build/cpp" ]; then
    echo "Nothing to do, cmake will generate protos as part of the build process."
elif [ "$BUILD_TARGET" == "build/go" ]; then
    echo "Generating Go protobufs..."
    cd ~/Catena/sdks/go
    make protos
fi
