package com.rossvideo.catena.example.device;

import java.io.File;

import com.rossvideo.catena.command.SimpleCommandHandler;
import com.rossvideo.catena.command.SimpleCommandStreamObserver;
import com.rossvideo.catena.device.BasicCatenaDevice;
import com.rossvideo.catena.device.CatenaServer;
import com.rossvideo.catena.device.impl.CommandManager;
import com.rossvideo.catena.device.impl.ConstraintUtils;
import com.rossvideo.catena.device.impl.ParamManager;
import com.rossvideo.catena.device.impl.ParamManager.WidgetHint;
import com.rossvideo.catena.example.device.command.FooCommandHandler;
import com.rossvideo.catena.example.device.command.ServerPushFileCommandHandler;
import com.rossvideo.catena.example.device.command.ServerReceiveFileCommandHandler;
import com.rossvideo.catena.example.error.UnknownOidException;

import catena.core.parameter.CommandResponse;
import catena.core.parameter.DataPayload;
import catena.core.parameter.ExecuteCommandPayload;
import catena.core.parameter.ParamType;
import catena.core.parameter.Value;
import io.grpc.stub.StreamObserver;

public class MyCatenaDevice extends BasicCatenaDevice {
    protected static final String CATENA_PRODUCT_NAME = "catena_product_name";
    protected static final String CATENA_DISPLAY_NAME = "catena_display_name";
    
    public static final String CHOICE_OID = "choice";
    public static final String INT_OID = "integer";
    public static final String FLOAT_OID = "float";
    public static final String CMD_FOO_OID = "foo";
    public static final String CMD_REVERSE_OID = "reverse";
    public static final String CMD_FILE_RECEIVE_OID = "file-receive";
    public static final String CMD_FILE_TRANSMIT_OID = "file-transmit";

    private File workingDirectory;

    public MyCatenaDevice(CatenaServer server, int slot, File workingDirectory) { 
        super(server, slot);
        this.workingDirectory = workingDirectory; 
        init();
    }
    
    private void init() {
        buildMenus();
        buildParams();
        buildCommands();
    }

    private void buildMenus() {
        getMenuManager().createMenuGroup("config", 1, "Config");
        getMenuManager().createMenu("config", "config", 0, "Configuration");
        getMenuManager().addParamsMenu("config", "config", new String[] {FLOAT_OID, INT_OID, CHOICE_OID});;
        
        getMenuManager().createMenuGroup("status", 0, "Status");
        getMenuManager().createMenu("status", "product", 0, "Product");
        getMenuManager().addParamsMenu("status", "product", new String[] {FLOAT_OID, INT_OID, CHOICE_OID});
    }
    
    private void buildParams() {
        ParamManager manager = getParamManager();
        manager.createParamDescriptor(CATENA_DISPLAY_NAME, "Display Name", ParamType.STRING, true, Value.newBuilder().setStringValue("Display Name").build(), null, WidgetHint.TEXT_DISPLAY);
        manager.addParamAlias(CATENA_DISPLAY_NAME, "0xFF01");
        manager.createParamDescriptor(CATENA_PRODUCT_NAME, "Product Name", ParamType.STRING, true, Value.newBuilder().setStringValue("Example Device").build());
        manager.addParamAlias(CATENA_PRODUCT_NAME, "0x105");
        manager.createParamDescriptor(FLOAT_OID, "Float Parameter", ParamType.FLOAT32, false, Value.newBuilder().setFloat32Value(0f).build());
        manager.createParamDescriptor(INT_OID, "Int Parameter", ParamType.INT32, false, Value.newBuilder().setInt32Value(0).build());
        manager.createParamDescriptor(CHOICE_OID, "Choice Parameter", ParamType.INT32, false, Value.newBuilder().setInt32Value(0).build(), ConstraintUtils.buildIntChoiceConstraint(new String[] {
                        "Choice 1", "Choice 2", "Choice 3"
                }));
    }
    
    private void buildCommands() {
        //createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value)
        CommandManager manager = getCommandManager();
        manager.createParamDescriptor(CMD_FOO_OID, "String Command", ParamType.STRING, false, Value.newBuilder().setStringValue("").build());
        manager.createParamDescriptor(CMD_REVERSE_OID, "Reverse String", ParamType.STRING, false, Value.newBuilder().setStringValue("").build());
        manager.createParamDescriptor(CMD_FILE_RECEIVE_OID, "File Receive", ParamType.DATA, false, Value.newBuilder().setDataPayload(DataPayload.newBuilder().build()).build());
        manager.createParamDescriptor(CMD_FILE_TRANSMIT_OID, "File Transmit", ParamType.STRING, false, Value.newBuilder().setStringValue("").build());
    }

    @Override
    public StreamObserver<ExecuteCommandPayload> executeCommand(ExecuteCommandPayload firstMessage, StreamObserver<CommandResponse> responseStream)
    {
        String oid = firstMessage.getOid();
        switch (oid)
        {
            case CMD_FOO_OID:
                return new FooCommandHandler(getSlot(), responseStream);
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
