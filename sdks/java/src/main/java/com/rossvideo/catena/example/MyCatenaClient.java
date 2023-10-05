package com.rossvideo.catena.example;

import com.rossvideo.catena.example.command.CommandResponseHandler;

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
import io.grpc.stub.StreamObserver;

public class MyCatenaClient implements AutoCloseable {
    private final String hostname;
    private final int port;
    private CatenaServiceBlockingStub blockingStub; // Used for unary and server side streaming calls
    private CatenaServiceStub asyncStub; // Used for client-side and bi-directional streaming calls
    private ManagedChannel channel;

    public MyCatenaClient(String hostname, int port) {
        this(hostname, port, 1);
    }

    public MyCatenaClient(String hostname, int port, int slotNumber) {
        this.hostname = hostname;
        this.port = port;
    }

    public void start() {
        if (channel == null) {
            ChannelCredentials credentials = InsecureChannelCredentials.create(); // TODO: Create proper credentials.
            channel = Grpc.newChannelBuilderForAddress(hostname, port, credentials).build();
            blockingStub = CatenaServiceGrpc.newBlockingStub(channel);
            asyncStub = CatenaServiceGrpc.newStub(channel);
        }
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
