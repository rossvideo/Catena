package com.rossvideo.catena.example.device;

import java.io.File;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;

import com.google.protobuf.Empty;
import com.google.protobuf.StringValue;
import com.rossvideo.catena.command.BidirectionalDelegatingStreamObserver;
import com.rossvideo.catena.command.BidirectionalStreamObserverFactory;
import com.rossvideo.catena.command.SimpleCommandHandler;
import com.rossvideo.catena.command.SimpleCommandStreamObserver;
import com.rossvideo.catena.example.device.command.FooCommandHandler;
import com.rossvideo.catena.example.device.command.ServerPushFileCommandHandler;
import com.rossvideo.catena.example.device.command.ServerReceiveFileCommandHandler;
import com.rossvideo.catena.example.error.InvalidSlotNumberException;
import com.rossvideo.catena.example.error.UnknownOidException;
import com.rossvideo.catena.example.error.WrongValueTypeException;

import catena.core.constraint.Constraint;
import catena.core.constraint.Constraint.ConstraintType;
import catena.core.constraint.Int32ChoiceConstraint;
import catena.core.constraint.Int32ChoiceConstraint.IntChoice;
import catena.core.device.ConnectPayload;
import catena.core.device.Device;
import catena.core.device.DeviceComponent;
import catena.core.device.DeviceRequestPayload;
import catena.core.device.PushUpdates;
import catena.core.device.PushUpdates.PushValue;
import catena.core.language.PolyglotText;
import catena.core.menu.Menu;
import catena.core.menu.MenuGroup;
import catena.core.menu.MenuGroups;
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
import io.grpc.stub.ServerCallStreamObserver;
import io.grpc.stub.StreamObserver;

public class MyCatenaDevice extends CatenaServiceImplBase implements BidirectionalStreamObserverFactory<ExecuteCommandPayload, CommandResponse> {
    protected static final String CATENA_PRODUCT_NAME = "catena_product_name";
    protected static final String CATENA_DISPLAY_NAME = "catena_display_name";
    
    public static final String CHOICE_OID = "choice";
    public static final String INT_OID = "integer";
    public static final String FLOAT_OID = "float";
    public static final String CMD_FOO_OID = "foo";
    public static final String CMD_REVERSE_OID = "reverse";
    public static final String CMD_FILE_RECEIVE_OID = "file-receive";
    public static final String CMD_FILE_TRANSMIT_OID = "file-transmit";

    private static final AtomicInteger deviceId = new AtomicInteger(1);

    private Value choiceValue = Value.newBuilder().setInt32Value(0).build();
    private Value intValue = Value.newBuilder().setInt32Value(0).build();
    private Value floatValue = Value.newBuilder().setFloat32Value(0F).build();

    private final String deviceName = MyCatenaDevice.class.getSimpleName() + '-' + deviceId.getAndIncrement();
    private final int slot;
    private File workingDirectory;
    private Set<StreamObserver<PushUpdates>> pushConnections = new LinkedHashSet<>(); 

    public MyCatenaDevice(int slot, File workingDirectory) { 
        this.slot = slot; 
        this.workingDirectory = workingDirectory; 
    }
    
    

    @Override
    public void connect(ConnectPayload request, StreamObserver<PushUpdates> responseObserver)
    {
        addConnection(responseObserver);
        Device device = buildDeviceMessage();
        DeviceComponent deviceComponent = DeviceComponent.newBuilder().setDevice(device).build();
        PushUpdates.Builder pushUpdate = PushUpdates.newBuilder()
                .setSlot(slot)
                .setDeviceComponent(deviceComponent);
        responseObserver.onNext(pushUpdate.build());
    }



    protected void addConnection(StreamObserver<PushUpdates> responseObserver)
    {
        synchronized (pushConnections)
        {
            if (pushConnections.add(responseObserver) && responseObserver instanceof ServerCallStreamObserver)
            {
                ((ServerCallStreamObserver<PushUpdates>) responseObserver).setOnCancelHandler(() -> {
                    removeConnection(responseObserver);
                });
            }
        }
    }



    protected void removeConnection(StreamObserver<PushUpdates> responseObserver)
    {
        synchronized (pushConnections)
        {
            pushConnections.remove(responseObserver);
            responseObserver.onCompleted();
        }
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

        Device device = buildDeviceMessage();
        DeviceComponent deviceComponent = DeviceComponent.newBuilder().setDevice(device).build();
        responseObserver.onNext(deviceComponent);
        responseObserver.onCompleted();
    }



    protected Device buildDeviceMessage()
    {
        Device device =
          Device.newBuilder().setSlot(this.slot).setMenuGroups(buildAllMenus())
          .putAllParams(buildAllParamDescriptors())
          .putAllCommands(buildAllCommandDescriptors())
          .build();
        return device;
    }

    private MenuGroups buildAllMenus() {
        Menu configMenu = Menu.newBuilder()
                .setName(PolyglotText.newBuilder().putDisplayStrings("en", "Config").build())
                .addParamOids(FLOAT_OID)
                .addParamOids(INT_OID)
                .addParamOids(CHOICE_OID)
                .setId(1)
                .build();
        MenuGroup configGroup = MenuGroup.newBuilder().putMenus("config", configMenu)
                .setId(1)
                .build();
        
        Menu statusMenu = Menu.newBuilder()
                .setName(PolyglotText.newBuilder().putDisplayStrings("en", "Product").build())
                .addParamOids(FLOAT_OID)
                .addParamOids(INT_OID)
                .addParamOids(CHOICE_OID)
                .setId(0)
                .build();
        MenuGroup statusGroup = MenuGroup.newBuilder().putMenus("status", statusMenu)
                .setId(0)
                .build();
        
        return MenuGroups.newBuilder()
                .putMenuGroups("status", statusGroup)
                .putMenuGroups("config", configGroup).build();
    }
    
    private Map<String, Param> buildAllParamDescriptors() {
        Map<String, Param> parameters = new HashMap<>();
        parameters.put(
                CATENA_DISPLAY_NAME, buildParamDescriptor(CATENA_DISPLAY_NAME, new String[]{"0xFF01"}, "Display Name", ParamType.STRING, true, 0, Value.newBuilder().setStringValue("Display Name").build(), null, "text-display"));
        parameters.put(
                CATENA_PRODUCT_NAME, buildParamDescriptor(CATENA_PRODUCT_NAME, new String[]{"0x105"}, "Product Name", ParamType.STRING, true, 0, Value.newBuilder().setStringValue("Example Device").build(), null, null));
        parameters.put(
          FLOAT_OID, buildParamDescriptor(FLOAT_OID, null, "Float Parameter", ParamType.FLOAT32, false, 1, floatValue, null, null));
        parameters.put(INT_OID,
                       buildParamDescriptor(INT_OID, null, "Int Parameter", ParamType.INT32, false, 1, intValue, null, null));
        parameters.put(CHOICE_OID,
                buildParamDescriptor(CHOICE_OID, null, "Choice Parameter", ParamType.INT32, false, 1, choiceValue, buildIntChoiceConstraint(new String[] {
                        "Choice 1", "Choice 2", "Choice 3"
                }), null));
        return parameters;
    }
    
    private Map<String, Param> buildAllCommandDescriptors() {
        Map<String, Param> commands = new HashMap<>();
        commands.put(
          CMD_FOO_OID, buildParamDescriptor(CMD_FOO_OID, null, "string command", ParamType.STRING, false, 1, Value.newBuilder().setStringValue("").build(), null, null, true));
        commands.put(
                CMD_FILE_RECEIVE_OID, buildParamDescriptor(CMD_FILE_RECEIVE_OID, null, "file receive command", ParamType.DATA, false, 1, Value.newBuilder().setDataPayload(DataPayload.newBuilder().build()).build(), null, null, true));
        commands.put(
                CMD_FILE_TRANSMIT_OID, buildParamDescriptor(CMD_FILE_RECEIVE_OID, null, "file transmit command", ParamType.EMPTY, false, 1, Value.newBuilder().setEmptyValue(Empty.getDefaultInstance()).build(), null, null, true));
        return commands;
    }
    
    private Param buildParamDescriptor(String oid, String[] aliases, String name, ParamType type, boolean readonly,
            int precision, Value value, Constraint constraint, String widget) {
                return buildParamDescriptor(oid, aliases, name, type, readonly, precision, value, constraint, widget, false);
    
    }
    
    private Param buildParamDescriptor(String oid, String[] aliases, String name, ParamType type, boolean readonly,
                                                 int precision, Value value, Constraint constraint, String widget, boolean response) {
        Param.Builder param = Param.newBuilder()
                        .setName(generateText(name))
                        .setType(type)
                        .setReadOnly(readonly)
                        .setPrecision(precision)
                        .setValue(value)
                        .setResponse(response);
        
        if (widget != null)
        {
            param.setWidget(widget);
        }
        
        if (aliases != null && aliases.length > 0)
        {
            param.addAllOidAliases(Arrays.asList(aliases));
        }
        
        if (constraint != null)
        {
            param.setConstraint(constraint);
        }
                        
        return param.build();
    }
    
    private PolyglotText generateText(String name)
    {
        return PolyglotText.newBuilder().putDisplayStrings("en", name).build();
    }



    private Constraint buildIntChoiceConstraint(String[] choices)
    {
        if (choices != null)
        {
            Constraint.Builder constraint = Constraint.newBuilder();
            Int32ChoiceConstraint.Builder intChoice = Int32ChoiceConstraint.newBuilder();
            for (int i = 0; i < choices.length; i++)
            {
                intChoice.addChoices(IntChoice.newBuilder()
                        .setValue(i)
                        .setName(generateText(choices[i])));
            }
            constraint.setType(ConstraintType.INT_CHOICE);
            constraint.setInt32Choice(intChoice);
            return constraint.build();
        }
        return null;
        
    }
    
    @Override
    public StreamObserver<ExecuteCommandPayload> executeCommand(StreamObserver<CommandResponse> responseObserver) {
        return new BidirectionalDelegatingStreamObserver<ExecuteCommandPayload, CommandResponse>(this, responseObserver);
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
            case CHOICE_OID:
                if (valueKindCase != KindCase.INT32_VALUE) {
                    responseObserver.onError(
                      new WrongValueTypeException(oid, valueKindCase, KindCase.INT32_VALUE));
                    return;
                }
                choiceValue = value;
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
        
        notifyClients(PushUpdates.newBuilder()
                .setSlot(slot)
                .setValue(PushValue.newBuilder()
                        .setOid(oid)
                        .setElementIndex(request.getElementIndex())
                        .setValue(value)
                        .build())
                .build());
    }
    
    protected void notifyClients(PushUpdates pushUpdates)
    {
        synchronized (pushConnections)
        {
            for (StreamObserver<PushUpdates> observer : pushConnections)
            {
                System.out.println("NOTIFYING " + observer);
                observer.onNext(pushUpdates);
            }
        }
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
