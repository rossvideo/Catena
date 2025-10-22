#!/bin/bash

if [ ! -d "$HOME/Catena/build/java" ]; then
    echo "Directory Catena/build/java does not exist."
    echo "Please build the Java project before running tests."
    echo "HINT: Use '~/Catena/sdks/java/scripts/build.sh' to build the Java project."
    exit 1
fi

cd ~/Catena/build/java

if [ $# -eq 0 ]; then
    echo "No arguments provided. Use '--help' for available options."
    exit 1
elif [ $# -gt 1 ]; then
    echo "Please only use one argument. Use '--help' for available options."
    exit 1
else
    for arg in "$@"; do
        case $arg in
            --server)
                echo "Running server test application."
                java -cp catena-java-0.0.1-SNAPSHOT-shaded.jar com.rossvideo.catena.example.main.ServerMain
                ;;
            --client)
                echo "Running client test application."
                java -cp catena-java-0.0.1-SNAPSHOT-shaded.jar com.rossvideo.catena.example.main.ClientMain
                ;;
            --server-client)
                echo "Running server-client test application."
                java -cp catena-java-0.0.1-SNAPSHOT-shaded.jar com.rossvideo.catena.example.main.ServerClientMain
                ;;
            --help)
                echo "Available options:"
                echo "  --server: Run the server test application."
                echo "  --client: Run the client test application."
                echo "  --server-client: Run the server-client test application."
                echo "  --help: Show this help message."
                ;;
            *)
                echo "Unknown argument: $arg"
                echo "Use '--help' for available options."
                exit 1
                ;;
        esac
    done
fi
exit 0
