package com.rossvideo.catena.example.device;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;
import java.util.function.BiConsumer;

import com.rossvideo.catena.datapayload.DataPayloadBuilder;
import com.rossvideo.catena.datapayload.FilePayloadReceiver;
import com.rossvideo.catena.example.error.WrongValueTypeException;

import catena.core.parameter.ExecuteCommandPayload;
import catena.core.parameter.CommandResponse;
import catena.core.parameter.DataPayload;
import catena.core.parameter.Value;
import catena.core.parameter.Value.KindCase;
import io.grpc.stub.StreamObserver;

public final class CommandHandler {

    private File workingDirectory;
    private Map<String, BiConsumer<ExecuteCommandPayload, StreamObserver<CommandResponse>>> commandMap = new HashMap<>();

    public static final String CMD_FOO_OID = "foo";
    public static final String CMD_REVERSE_OID = "reverse";
    public static final String CMD_FILE_RECEIVE_OID = "file-receive";
    public static final String CMD_FILE_TRANSMIT_OID = "file-transmit";

    public CommandHandler(File workingDirectory) {
        this.workingDirectory = workingDirectory;

        commandMap.put(CMD_FOO_OID, this::fooCommand);
        commandMap.put(CMD_REVERSE_OID, this::reverseCommand);
        commandMap.put(CMD_FILE_RECEIVE_OID, this::recieveFileCommand);
        commandMap.put(CMD_FILE_TRANSMIT_OID, this::pushFileCommand);
    }

    public void executeCommand(ExecuteCommandPayload request, StreamObserver<CommandResponse> responseObserver) {
        String oid = request.getOid();

        BiConsumer<ExecuteCommandPayload, StreamObserver<CommandResponse>> commandHandler = commandMap.get(oid);
        if (commandHandler == null) {
            responseObserver.onError(new IllegalArgumentException("Unknown command: " + oid));
            return;
        }

        try {
            commandHandler.accept(request, responseObserver);
        } catch (WrongValueTypeException e) {
            responseObserver.onError(e);
        } catch (Exception e) {
            responseObserver.onError(new RuntimeException("Error executing command: " + oid, e));
        }
    }

    private void fooCommand(ExecuteCommandPayload request, StreamObserver<CommandResponse> responseObserver)
    {
        Value value = request.getValue();
        KindCase valueKindCase = value.getKindCase();
        if (valueKindCase != KindCase.STRING_VALUE) {
            throw new WrongValueTypeException("foo", valueKindCase, KindCase.STRING_VALUE);
        }

        System.out.println("Executing foo command for String: \"" + value.getStringValue() + "\"");

        if (request.getRespond()) {
            String responseString = "YOU SENT \"" + value.getStringValue() + "\"";
            responseObserver.onNext(
                CommandResponse.newBuilder()
                        .setResponse(Value.newBuilder().setStringValue(responseString).build())
                        .build());
        }
        responseObserver.onCompleted();
    }
    
    private void reverseCommand(ExecuteCommandPayload request, StreamObserver<CommandResponse> responseObserver)
    {
        Value value = request.getValue();
        KindCase valueKindCase = value.getKindCase();
        if (valueKindCase != KindCase.STRING_VALUE) {
            throw new WrongValueTypeException("reverse", valueKindCase, KindCase.STRING_VALUE);
        }

        String reversedString = new StringBuilder(value.getStringValue()).reverse().toString();
        System.out.println("Executing reverse command for String: \"" + value.getStringValue() + "\". Reversed: \"" + reversedString + "\"");

        if (request.getRespond()) {
            responseObserver.onNext(
                CommandResponse.newBuilder()
                        .setResponse(Value.newBuilder().setStringValue(reversedString).build())
                        .build());
        }
        responseObserver.onCompleted();
    }

    private void recieveFileCommand(ExecuteCommandPayload request, StreamObserver<CommandResponse> responseObserver) {
        FilePayloadReceiver filePayloadReceiver = new FilePayloadReceiver(workingDirectory, "new file");
        Value value = request.getValue();
        if (value == null || value.getDataPayload() == null)
        {
            responseObserver.onError(new IllegalArgumentException("No data payload provided."));
            return;
        }
        
        try
        {
            DataPayload dataPayload = value.getDataPayload();
            filePayloadReceiver.init(dataPayload);
            String fileName = filePayloadReceiver.getCurrentFile().getName();
            System.out.println("SERVER: recieved file: " + fileName);
            filePayloadReceiver.close();

            if (request.getRespond())
            {
                responseObserver.onNext(
                    CommandResponse.newBuilder()
                        .setResponse(Value.newBuilder().setStringValue("Server recieved" + fileName))
                        .build());
            }
            responseObserver.onCompleted();
        }
        catch (Exception e)
        {
            System.err.println("Error receiving file: " + e.getMessage());
            responseObserver.onError(e);
        }
    }

    private void pushFileCommand(ExecuteCommandPayload request, StreamObserver<CommandResponse> responseObserver) {
        DataPayloadBuilder payloadBuilder = null;
        
        try 
        {
            String resource = "sample-file.jpg";
            payloadBuilder = new DataPayloadBuilder(createInputStream(resource), resource, "image/jpeg");
            payloadBuilder.calculateDigest();
            while (payloadBuilder.hasNext())
            {
                sendNext(responseObserver, payloadBuilder);
            }
            payloadBuilder.close();
            responseObserver.onCompleted();
        }
        catch (Exception e)
        {
            System.err.println("SERVER: Error pushing file: " + e.getMessage());
            if (payloadBuilder != null)
            {
                payloadBuilder.close();
            }
            responseObserver.onError(e);
        }
    }

    private InputStream createInputStream(String resource) throws IOException
    {
        return MyCatenaDevice.class.getResourceAsStream("/files/" + resource);
    }

    private void sendNext(StreamObserver<CommandResponse> responseObserver, DataPayloadBuilder payloadBuilder) throws IOException
    {
        responseObserver.onNext(
            CommandResponse.newBuilder()
                .setResponse(Value.newBuilder().setDataPayload(payloadBuilder.buildNext()).build())
                .build());
    }
}
