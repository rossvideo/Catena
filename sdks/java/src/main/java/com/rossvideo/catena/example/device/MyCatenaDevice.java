package com.rossvideo.catena.example.device;

import java.io.File;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;

import com.google.protobuf.Empty;
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
import catena.core.parameter.SetValuePayload;
import catena.core.parameter.Value;
import io.grpc.stub.StreamObserver;

public class MyCatenaDevice extends BasicCatenaDevice {
    
    protected static final String CATENA_PRODUCT_NAME = "catena_product_name";
    protected static final String CATENA_DISPLAY_NAME = "catena_display_name";
    
    public static final String CHOICE_OID = "/parameters/nested/choice";
    public static final String STRUCT_OID = "/product/serial-number";
    public static final String INT_OID = "/parameters/nested/integer";
    public static final String FLOAT_OID = "/parameters/nested/float";
    public static final String FLOAT_OID_RANGE = "/parameters/nested/float_range";
    public static final String DATE_AND_TIME_OID = "/clock/date-and-time";
    public static final String CLOCK_ON_OID = "/clock/on";
    
    public static final String CMD_FOO_OID = "foo";
    public static final String CMD_REVERSE_OID = "reverse";
    public static final String CMD_FILE_RECEIVE_OID = "file-receive";
    public static final String CMD_FILE_TRANSMIT_OID = "file-transmit";

    private File workingDirectory;
    private DateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
    private Timer clockTimer = new Timer();
    private volatile boolean clockUpdating = false;

    public MyCatenaDevice(CatenaServer server, int slot, File workingDirectory) { 
        super(server, slot);
        this.workingDirectory = workingDirectory; 
        init();
    }
    
    private void init() {
        buildMenus();
        buildParams();
        buildCommands();
        startTimeUpdate();
    }

    private void buildMenus() {
        getMenuManager().createMenuGroup("config", "Config");
        getMenuManager().createMenu("config", "config", "Configuration");
        getMenuManager().addParamsMenu("config", "config", new String[] {FLOAT_OID, FLOAT_OID_RANGE, INT_OID, CHOICE_OID});
        getMenuManager().createMenu("config", "clock", "Clock");
        getMenuManager().addParamsMenu("config", "clock", new String[] {DATE_AND_TIME_OID, CLOCK_ON_OID});
        
        getMenuManager().createMenuGroup("status", "Status");
        getMenuManager().createMenu("status", "product", "Product");
        getMenuManager().addParamsMenu("status", "product", new String[] {STRUCT_OID, DATE_AND_TIME_OID, FLOAT_OID, INT_OID, CHOICE_OID});
    }
    
    private String getTime()
    {
        return dateFormat.format(new Date());
    }
    
    private void updateTime()
    {
        Value clockOn = getParamManager().getValue(CLOCK_ON_OID, 0);
        boolean shouldUpdate = clockOn == null ? false : clockOn.getInt32Value() != 0;
        if (!shouldUpdate)
        {
            if (clockUpdating)
            {
                clockTimer.cancel();
                clockTimer = new Timer();
                clockUpdating = false;
            }
            return;
        }
        else if (!clockUpdating)
        {
            startTimeUpdate();
        }
        
        String time = getTime();
        try
        {
            setValue(DATE_AND_TIME_OID, 0, Value.newBuilder().setStringValue(time).build());
        }
        catch (UnknownOidException ex)
        {
            ex.printStackTrace();
        }
    }
    
    
    
    @Override
    public void setValue(SetValuePayload request, StreamObserver<Empty> responseObserver)
    {
        super.setValue(request, responseObserver);
        if (request.getOid().equals(CLOCK_ON_OID))
        {
            updateTime();
        }
    }

    private void startTimeUpdate()
    {
        clockUpdating = true;
        clockTimer.scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run()
            {
                updateTime();
            }
        }, 500, 500);
    }
    
    private void buildParams() {
        ParamManager manager = getParamManager();
        manager.createParamDescriptor(CATENA_DISPLAY_NAME, "Display Name", ParamType.STRING, true, Value.newBuilder().setStringValue("Display Name").build(), null, WidgetHint.TEXT_DISPLAY)
            .addOidAliases("0xFF01");
        manager.createParamDescriptor(CATENA_PRODUCT_NAME, "Product Name", ParamType.STRING, true, Value.newBuilder().setStringValue("Example Device").build())
            .addOidAliases("0x105");
        manager.createParamDescriptor(STRUCT_OID, "Serial Number", ParamType.STRING, true, Value.newBuilder().setStringValue("12-345-6989").build(), null, WidgetHint.TEXT_DISPLAY);
        manager.createParamDescriptor(DATE_AND_TIME_OID, "Date and Time", ParamType.STRING, true, Value.newBuilder().setStringValue(getTime()).build(), null, WidgetHint.LABEL);
        manager.createParamDescriptor(CLOCK_ON_OID, "Clock On", ParamType.INT32, false, Value.newBuilder().setInt32Value(0).build(), ConstraintUtils.buildIntChoiceConstraint(new String[] {
                "Off", "On"
        }), WidgetHint.CHECKBOX);
        manager.createParamDescriptor(FLOAT_OID, "Float Parameter", ParamType.FLOAT32, false, Value.newBuilder().setFloat32Value(0f).build());
        manager.createParamDescriptor(FLOAT_OID_RANGE, "Float Range Parameter", ParamType.FLOAT32, false, Value.newBuilder().setFloat32Value(0f).build(), ConstraintUtils.buildFloatRangeConstraint(0f, 100f, 0.1f));
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
