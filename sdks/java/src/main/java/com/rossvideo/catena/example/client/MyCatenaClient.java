package com.rossvideo.catena.example.client;

import java.io.File;
import java.io.IOException;
import java.net.URL;

import javax.net.ssl.SSLException;

import com.rossvideo.catena.example.client.command.ClientPushFileResponseHandler;
import com.rossvideo.catena.example.client.command.ClientReceiveFileResponseHandler;
import com.rossvideo.catena.example.client.command.CommandResponseHandler;

import catena.core.device.DeviceRequestPayload;
import catena.core.parameter.ExecuteCommandPayload;
import catena.core.parameter.GetValuePayload;
import catena.core.parameter.SetValuePayload;
import catena.core.parameter.Value;
import catena.core.service.CatenaServiceGrpc;
import catena.core.service.CatenaServiceGrpc.CatenaServiceBlockingStub;
import catena.core.service.CatenaServiceGrpc.CatenaServiceStub;
import io.grpc.ChannelCredentials;
import io.grpc.Grpc;
import io.grpc.InsecureChannelCredentials;
import io.grpc.ManagedChannel;
import io.grpc.StatusRuntimeException;
import io.grpc.netty.shaded.io.grpc.netty.GrpcSslContexts;
import io.grpc.netty.shaded.io.grpc.netty.NettyChannelBuilder;
import io.grpc.netty.shaded.io.netty.handler.ssl.SslContext;
import io.grpc.stub.StreamObserver;

public class MyCatenaClient implements AutoCloseable {
    private final String hostname;
    private final int port;
    private CatenaServiceBlockingStub blockingStub; // Used for unary and server side streaming calls
    private CatenaServiceStub asyncStub; // Used for client-side and bi-directional streaming calls
    private ManagedChannel channel;
    private File workingDirectory;
    private boolean secure;

    public MyCatenaClient(String hostname, int port, File workingDirectory, boolean secure) {
        this.hostname = hostname;
        this.port = port;
        this.secure = secure;
        setWorkingDirectory(workingDirectory);
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
            asyncStub = CatenaServiceGrpc.newStub(channel);
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
        asyncStub = null;
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
            blockingStub
                    .setValue(SetValuePayload.newBuilder().setOid(oid).setSlot(slotNumber).setValue(newValue).build());
        } catch (StatusRuntimeException exception) {
            printStatusRuntimeException("setValue", exception);
        }
    }

    public void executeCommand(String oid, int slot, int value, boolean respond) {
        executeCommand(oid, slot, Value.newBuilder().setInt32Value(value).build(), respond);
    }

    public void executeCommand(String oid, int slot, float value, boolean respond) {
        executeCommand(oid, slot, Value.newBuilder().setFloat32Value(value).build(), respond);
    }

    public void executeCommand(String oid, int slot, String value, boolean respond) {
        executeCommand(oid, slot, Value.newBuilder().setStringValue(value).build(), respond);
    }

    public void executeCommand(String oid, int slot, Value value, boolean respond) {
        StreamObserver<ExecuteCommandPayload> commandPayloadStream = asyncStub
                .executeCommand(new CommandResponseHandler());

        try {
            // TODO: use a for-loop if you have more than one ExecuteCommandPayload to send.
            commandPayloadStream.onNext(ExecuteCommandPayload.newBuilder().setOid(oid).setSlot(slot).setValue(value)
                    .setRespond(respond).build());
            commandPayloadStream.onCompleted();
        } catch (StatusRuntimeException e) {
            printStatusRuntimeException("executeCommand", e);
            commandPayloadStream.onError(e);
        }
    }
    
    public void receiveFile(String oid, int slotNumber, String filename) throws InterruptedException, IOException {
        ClientReceiveFileResponseHandler receiveHandler = new ClientReceiveFileResponseHandler(oid, slotNumber, getWorkingDirectory(), this);
        synchronized (this)
        {
            StreamObserver<ExecuteCommandPayload> commandPayloadStream = asyncStub.executeCommand(receiveHandler);
            receiveHandler.setTxStream(commandPayloadStream);
            this.wait(60000);
        }
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

    public void pushFile(String oid, int slot, URL file) throws InterruptedException {
        pushFile(oid, slot, new URL[] { file });
    }
    
    public void pushFile(String oid, int slot, URL[] files) throws InterruptedException {
        ClientPushFileResponseHandler push = new ClientPushFileResponseHandler(slot, oid, files, this);
        synchronized (this)
        {
            StreamObserver<ExecuteCommandPayload> commandPayloadStream = asyncStub.executeCommand(push);
            try
            {
                push.setTxStream(commandPayloadStream);
            }
            catch (IOException e)
            {
                e.printStackTrace();
            }
            this.wait(60000);
        }
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
