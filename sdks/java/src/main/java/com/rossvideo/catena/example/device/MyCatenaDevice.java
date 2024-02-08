package com.rossvideo.catena.example.device;

import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import com.google.protobuf.Empty;
import com.rossvideo.catena.command.DelegatingStreamObserver;
import com.rossvideo.catena.command.SimpleCommandHandler;
import com.rossvideo.catena.command.SimpleCommandStreamObserver;
import com.rossvideo.catena.command.StreamObserverFactory;
import com.rossvideo.catena.example.device.command.ServerReceiveFileCommandHandler;
import com.rossvideo.catena.example.device.command.ServerPushFileCommandHandler;
import com.rossvideo.catena.example.device.command.FooCommandHandler;
import com.rossvideo.catena.example.error.InvalidSlotNumberException;
import com.rossvideo.catena.example.error.UnknownOidException;
import com.rossvideo.catena.example.error.WrongValueTypeException;

import catena.core.device.Device;
import catena.core.device.DeviceComponent;
import catena.core.device.DeviceRequestPayload;
import catena.core.language.PolyglotText;
import catena.core.parameter.CommandResponse;
import catena.core.parameter.DataPayload;
import catena.core.parameter.ExecuteCommandPayload;
import catena.core.parameter.GetValuePayload;
import catena.core.parameter.Param;
import catena.core.parameter.ParamType;
import catena.core.parameter.SetValuePayload;
import catena.core.parameter.Value;
import catena.core.parameter.Value.KindCase;
import catena.core.service.CatenaServiceGrpc.CatenaServiceImplBase;
import io.grpc.stub.StreamObserver;

public class MyCatenaDevice extends CatenaServiceImplBase implements StreamObserverFactory<ExecuteCommandPayload, CommandResponse> {
    public static final String INT_OID = "integer";
    public static final String FLOAT_OID = "float";
    public static final String CMD_FOO_OID = "foo";
    public static final String CMD_REVERSE_OID = "reverse";
    public static final String CMD_FILE_RECEIVE_OID = "file-receive";
    public static final String CMD_FILE_TRANSMIT_OID = "file-transmit";

    private static final AtomicInteger deviceId = new AtomicInteger(1);

    private Value intValue = Value.newBuilder().setInt32Value(0).build();
    private Value floatValue = Value.newBuilder().setFloat32Value(0F).build();

    private final String deviceName = MyCatenaDevice.class.getSimpleName() + '-' + deviceId.getAndIncrement();
    private final int slot;
    private File workingDirectory;

    public MyCatenaDevice(int slot, File workingDirectory) { 
        this.slot = slot; 
        this.workingDirectory = workingDirectory; 
    }

    @Override
    public void deviceRequest(DeviceRequestPayload request,
                              StreamObserver<DeviceComponent> responseObserver) {
        int slot = request.getSlot();
        System.out.println("SERVER: deviceRequest request for slot: " + slot);
        if (this.slot != slot) {
            responseObserver.onError(new InvalidSlotNumberException(slot));
            return;
        }

        Device device =
          Device.newBuilder().setSlot(this.slot).putAllParams(buildAllParamDescriptors())
          .putAllCommands(buildAllCommandDescriptors())
          .build();
        DeviceComponent deviceComponent = DeviceComponent.newBuilder().setDevice(device).build();
        responseObserver.onNext(deviceComponent);
        responseObserver.onCompleted();
    }

    private Map<String, Param> buildAllParamDescriptors() {
        Map<String, Param> parameters = new HashMap<>();
        parameters.put(
          FLOAT_OID, buildParamDescriptor(FLOAT_OID, "float parameter", ParamType.FLOAT32, false, 1, floatValue));
        parameters.put(INT_OID,
                       buildParamDescriptor(INT_OID, "int parameter", ParamType.INT32, false, 1, intValue));
        return parameters;
    }
    
    private Map<String, Param> buildAllCommandDescriptors() {
        Map<String, Param> commands = new HashMap<>();
        commands.put(
          CMD_FOO_OID, buildParamDescriptor(CMD_FOO_OID, "string command", ParamType.STRING, false, 1, Value.newBuilder().setStringValue("").build(), true));
        commands.put(
                CMD_FILE_RECEIVE_OID, buildParamDescriptor(CMD_FILE_RECEIVE_OID, "file receive command", ParamType.DATA, false, 1, Value.newBuilder().setDataPayload(DataPayload.newBuilder().build()).build(), true));
        commands.put(
                CMD_FILE_TRANSMIT_OID, buildParamDescriptor(CMD_FILE_RECEIVE_OID, "file transmit command", ParamType.EMPTY, false, 1, Value.newBuilder().setEmptyValue(Empty.getDefaultInstance()).build(), true));
        return commands;
    }
    
    private Param buildParamDescriptor(String oid, String name, ParamType type, boolean readonly,
            int precision, Value value) {
                return buildParamDescriptor(oid, name, type, readonly, precision, value, false);
    
    }
    
    private Param buildParamDescriptor(String oid, String name, ParamType type, boolean readonly,
                                                 int precision, Value value, boolean response) {
        Param param = Param.newBuilder()
                        .setName(PolyglotText.newBuilder().putDisplayStrings("en", name).build())
                        .setType(type)
                        .setReadOnly(readonly)
                        .setPrecision(precision)
                        .setValue(value)
                        .setResponse(response)
                        .build();
        return param;
    }
    
    @Override
    public StreamObserver<ExecuteCommandPayload> executeCommand(StreamObserver<CommandResponse> responseObserver) {
        return new DelegatingStreamObserver<ExecuteCommandPayload, CommandResponse>(this, responseObserver);
    }

    @Override
    public void setValue(SetValuePayload request, StreamObserver<Empty> responseObserver) {
        System.out.println("SERVER: setValue request:");
        System.out.println("\tslot:  " + request.getSlot());
        System.out.println("\toid:   " + request.getOid());
        System.out.println("\tvalue: " + request.getValue());
        int slot = request.getSlot();
        if (this.slot != slot) {
            responseObserver.onError(new InvalidSlotNumberException(slot));
            return;
        }

        String oid = request.getOid();
        Value value = request.getValue();
        KindCase valueKindCase = value.getKindCase();
        switch (oid) {
            case INT_OID:
                if (valueKindCase != KindCase.INT32_VALUE) {
                    responseObserver.onError(
                      new WrongValueTypeException(oid, valueKindCase, KindCase.INT32_VALUE));
                    return;
                }
                intValue = value;
                break;
            case FLOAT_OID:
                if (valueKindCase != KindCase.FLOAT32_VALUE) {
                    responseObserver.onError(
                      new WrongValueTypeException(oid, valueKindCase, KindCase.FLOAT32_VALUE));
                    return;
                }
                floatValue = value;
                break;
            default:
                responseObserver.onError(new UnknownOidException(oid));
                return;
        }

        responseObserver.onNext(Empty.getDefaultInstance());
        responseObserver.onCompleted();
    }

    @Override
    public void getValue(GetValuePayload request, StreamObserver<Value> responseObserver) {
        System.out.println("SERVER: getValue request:");
        System.out.println("\toid:  " + request.getOid());
        System.out.println("\tslot: " + request.getSlot());
        System.out.println();
        int slot = request.getSlot();
        if (this.slot != slot) {
            responseObserver.onError(new InvalidSlotNumberException(slot));
            return;
        }

        Value value;
        String oid = request.getOid();
        switch (oid) {
            case INT_OID:
                value = intValue;
                break;
            case FLOAT_OID:
                value = floatValue;
                break;
            default:
                responseObserver.onError(new UnknownOidException(oid));
                return;
        }

        responseObserver.onNext(value);
        responseObserver.onCompleted();
    }

    @Override
    public StreamObserver<ExecuteCommandPayload> createStreamObserver(ExecuteCommandPayload firstMessage, StreamObserver<CommandResponse> responseStream)
    {
        String oid = firstMessage.getOid();
        switch (oid)
        {
            case CMD_FOO_OID:
                return new FooCommandHandler(slot, responseStream);
            case CMD_FILE_RECEIVE_OID:
                return new ServerReceiveFileCommandHandler(getWorkingDirectory(), responseStream);
            case CMD_FILE_TRANSMIT_OID:
                return new ServerPushFileCommandHandler(responseStream);
            case CMD_REVERSE_OID:
                return new SimpleCommandStreamObserver(responseStream, new SimpleCommandHandler()
                        {
                            @Override
                            public CommandResponse processCommand(ExecuteCommandPayload commandExecution) throws Exception
                            {
                                if (!commandExecution.hasValue() || !commandExecution.getValue().hasStringValue())
                                {
                                    throw new IllegalArgumentException("No string value provided.");
                                }
                                String value = commandExecution.getValue().getStringValue();
                                StringBuilder sb = new StringBuilder(value);
                                sb.reverse();
                                return CommandResponse.newBuilder().setResponse(Value.newBuilder().setStringValue(sb.toString())).build();
                            }
                        });
            default:
                throw new UnknownOidException(oid);
        }
    }

    private File getWorkingDirectory()
    {
        if (workingDirectory == null)
        {
            workingDirectory = new File(System.getProperty("user.dir"), "server-rx");
            workingDirectory.mkdirs();
        }
        
        return workingDirectory;
    }
}
