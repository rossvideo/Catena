package com.rossvideo.catena.example.client;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Iterator;

import javax.net.ssl.SSLException;

import com.rossvideo.catena.datapayload.DataPayloadBuilder;

import catena.core.device.DeviceRequestPayload;
import catena.core.parameter.CommandResponse;
import catena.core.parameter.DataPayload;
import catena.core.parameter.ExecuteCommandPayload;
import catena.core.parameter.GetValuePayload;
import catena.core.parameter.SetValuePayload;
import catena.core.parameter.SingleSetValuePayload;
import catena.core.parameter.Value;
import catena.core.service.CatenaServiceGrpc;
import catena.core.service.CatenaServiceGrpc.CatenaServiceBlockingStub;
import io.grpc.ChannelCredentials;
import io.grpc.Grpc;
import io.grpc.InsecureChannelCredentials;
import io.grpc.ManagedChannel;
import io.grpc.StatusRuntimeException;
import io.grpc.netty.shaded.io.grpc.netty.GrpcSslContexts;
import io.grpc.netty.shaded.io.grpc.netty.NettyChannelBuilder;
import io.grpc.netty.shaded.io.netty.handler.ssl.SslContext;

public class MyCatenaClient implements AutoCloseable {
    private final String hostname;
    private final int port;
    private CatenaServiceBlockingStub blockingStub; // Used for unary and server side streaming calls
    private ManagedChannel channel;
    private File workingDirectory;
    private boolean secure;
    private CommandResponseHandler commandResponseHandler;

    public MyCatenaClient(String hostname, int port, File workingDirectory, boolean secure) {
        this.hostname = hostname;
        this.port = port;
        this.secure = secure;
        setWorkingDirectory(workingDirectory);
        commandResponseHandler = new CommandResponseHandler(workingDirectory);
    }
    
    public void setWorkingDirectory(File workingDirectory)
    {
        this.workingDirectory = workingDirectory;
    }

    public void start() throws SSLException {
        if (channel == null) {
            // Specify the path to your server's certificate
            if (secure)
            {
                createSecureChannel();
            }
            else
            {
                createInsecureChannel();
            }

            
            blockingStub = CatenaServiceGrpc.newBlockingStub(channel);
        }
    }

    protected void createInsecureChannel()
    {
        ChannelCredentials credentials = InsecureChannelCredentials.create();
        channel = Grpc.newChannelBuilderForAddress(hostname, port, credentials).build();
    }

    protected void createSecureChannel() throws SSLException
    {
        File serverCertFile = null;
        try
        {
            serverCertFile = new File(getWorkingDirectory(), "server.crt");
        }
        catch (IOException e)
        {
        }
        
        // Create a gRPC channel with TLS support
        SslContext sslContext = GrpcSslContexts.forClient()
                .trustManager(serverCertFile)
                .build();
        
        channel = NettyChannelBuilder.forAddress(hostname, port)
                .sslContext(sslContext)
                .build();
    }

    @Override
    public void close() throws Exception {
        System.out.println("CLIENT: Shutting down Client.");
        if (channel != null) {
            channel.shutdownNow();
        }
        channel = null;
        blockingStub = null;
    }

    public void setValue(String oid, int slotNumber, int value) {
        printSetValueMessage("int", value);
        setValue(oid, slotNumber, Value.newBuilder().setInt32Value(value).build());
    }

    public void setValue(String oid, int slotNumber, float value) {
        printSetValueMessage("float", value);
        setValue(oid, slotNumber, Value.newBuilder().setFloat32Value(value).build());
    }

    public void setValue(String oid, int slotNumber, Value newValue) {
        try {
            blockingStub.setValue(SingleSetValuePayload.newBuilder()
                            .setSlot(slotNumber)
                            .setValue(SetValuePayload.newBuilder().setOid(oid).setValue(newValue))
                            .build());
        } catch (StatusRuntimeException exception) {
            printStatusRuntimeException("setValue", exception);
        }
    }

    public void executeCommand(String oid, int slot, int value, boolean respond) {
        try {
            Value requestValue = Value.newBuilder().setInt32Value(value).build();
            Iterator<CommandResponse>  responses = executeCommand(oid, slot, requestValue, respond);
            responses.forEachRemaining(commandResponseHandler::printCommandResponse);
        } catch (StatusRuntimeException exception) {
            printStatusRuntimeException("executeCommand", exception);
        }
    }

    public void executeCommand(String oid, int slot, float value, boolean respond) {
        try {
            Value requestValue = Value.newBuilder().setFloat32Value(value).build();
            Iterator<CommandResponse>  responses = executeCommand(oid, slot, requestValue, respond);
            responses.forEachRemaining(commandResponseHandler::printCommandResponse);
        } catch (StatusRuntimeException exception) {
            printStatusRuntimeException("executeCommand", exception);
        }
    }

    public void executeCommand(String oid, int slot, String value, boolean respond) {
        try {
            Value requestValue = Value.newBuilder().setStringValue(value).build();
            Iterator<CommandResponse>  responses = executeCommand(oid, slot, requestValue, respond);
            responses.forEachRemaining(commandResponseHandler::printCommandResponse);
        } catch (StatusRuntimeException exception) {
            printStatusRuntimeException("executeCommand", exception);
        }
    }

    public void receiveFile(String oid, int slotNumber, String filename) {
        try {
            Value requestValue = Value.newBuilder().setStringValue(filename).build();
            Iterator<CommandResponse> responses = executeCommand(oid, slotNumber, requestValue, true);
            commandResponseHandler.receiveFile(responses);
        } catch (StatusRuntimeException exception) {
            printStatusRuntimeException("receiveFile", exception);
        }
    }

    public void pushFile(String oid, int slot, URL file) {
        try {
            InputStream in = file.openStream();
            DataPayloadBuilder dataPayloadBuilder = new DataPayloadBuilder(in, getName(file), "image/jpeg");
            DataPayload dataPayload = dataPayloadBuilder.createCompleteFileDataPayload();      
            Value requestValue = Value.newBuilder().setDataPayload(dataPayload).build();
            Iterator<CommandResponse> responses = executeCommand(oid, slot, requestValue, false);
            responses.forEachRemaining((CommandResponse) -> {
                // No response expected, just consume the stream
            });
        } catch (IOException e) {
            System.err.println("CLIENT: Error creating file value: " + e.getMessage());
        } catch (StatusRuntimeException exception) {
            printStatusRuntimeException("pushFile", exception);
        }
    }

    private Iterator<CommandResponse>  executeCommand(String oid, int slot, Value value, boolean respond) throws StatusRuntimeException {
        return blockingStub.executeCommand(
                ExecuteCommandPayload.newBuilder()
                .setOid(oid)
                .setSlot(slot)
                .setValue(value)
                .setRespond(respond)
                .build());
    }
    
    private File getWorkingDirectory() throws IOException
    {
        if (workingDirectory == null)
        {
            createWorkingDirectory();
        }
        return workingDirectory;
    }

    private void createWorkingDirectory() throws IOException
    {
        workingDirectory = new File(System.getProperty("user.dir"), "client-rx");
        workingDirectory.mkdirs();
    }

    private static String getName(URL url)
    {
        String path = url.getPath();
        int index = path.lastIndexOf('/');
        if (index >= 0)
        {
            path = path.substring(index + 1);
        }
        return path;
    }
    

    public Value getValue(String oid, int slotNumber) {
        try {
            Value value = blockingStub.getValue(GetValuePayload.newBuilder().setOid(oid).setSlot(slotNumber).build());
            printGetValueResult("Value", value);
            return value;
        } catch (StatusRuntimeException exception) {
            printStatusRuntimeException("getValue", exception);
        }
        return null;
    }

    public void getDevice(int slotNumber) {
        try {
            blockingStub.deviceRequest(DeviceRequestPayload.newBuilder().setSlot(slotNumber).build())
              .forEachRemaining(
                deviceComponent -> { printGetValueResult("DeviceComponent", deviceComponent); });

        } catch (StatusRuntimeException exception) {
            printStatusRuntimeException("getDevice", exception);
        }
    }

    private void printSetValueMessage(String valueType, Object value) {
        System.out.println("CLIENT: Setting " + valueType + " value: " + value);
    }

    private void printGetValueResult(String valueType, Object result) {
        System.out.println("CLIENT: Received " + valueType + ": " + result);
    }

    private void printStatusRuntimeException(String methodName, StatusRuntimeException exception) {
        System.err.println("CLIENT: " + methodName + " RPC failed: " + exception.getStatus() + '\n');
        System.err.flush();
    }
}
