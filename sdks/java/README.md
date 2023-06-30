# Catena Java test application

This directory contains a maven project which:
1. Compiles the .proto files from Catena/interface directory to Java code.  
*Generated code is located in: Catena/sdks/java/target/generated-sources/protobuf/java*
2. Compiles the com.rossvideo example code.
3. Creates a catena-java-x.y.z-SNAPSHOT.jar files with all the compiled classes.
4. Creates an uber-jar catena-java-x.y.z-SNAPSHOT-shaded.jar with all the compiled and dependency classes.  
*This allows for test applications to be ran in a stand-alone fashion without specifying a classpath.*

## Requirements
1. Java 8 or higher installed on the computer.
2. The latest version of [maven](https://maven.apache.org/download.cgi) installed.

## Compiling and generating the jars
Simply run: `mvn clean package` in the Catena/sdks/java directory.  
Resulting jars will be in Catena/sdks/java/target directory.

## Running the test applications

### Running the server-only test application (Device simulator)
The Client application connect to a server on the localhost and performs some predefined operations on it.

1. Open a terminal
2. Navigate to `Catena/sdks/java/target`
3. Run `java -cp target/catena-java-0.0.1-SNAPSHOT-shaded.jar com.rossvideo.catena.example.main.ServerMain`  
The server will wait for a connection from a client.  Use Ctrl+C to exit.

### Running the client-only test application
The Client application connect to a server on the localhost and performs some predefined operations on it.

1. Open a terminal
2. Navigate to `Catena/sdks/java/target`
3. Run `java -cp target/catena-java-0.0.1-SNAPSHOT-shaded.jar com.rossvideo.catena.example.main.ClientMain`  
The Client will try to connect to the server, perform operations and exit.

### Running the Server-Client test application
This single application launches in separate threads, a server and a client.  The client connects to the server to perform predefined operations.  
Once complete the server and client are shutdown.
1. Open a terminal
2. Navigate to `Catena/sdks/java/target`
3. Run `java -cp target/catena-java-0.0.1-SNAPSHOT-shaded.jar com.rossvideo.catena.example.main.ServerClientMain`  


