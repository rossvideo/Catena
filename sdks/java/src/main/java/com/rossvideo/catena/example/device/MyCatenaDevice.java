package com.rossvideo.catena.example.device;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

import com.google.protobuf.ByteString;
import com.rossvideo.catena.device.BasicCatenaDevice;
import com.rossvideo.catena.device.CatenaServer;
import com.rossvideo.catena.device.impl.CommandManager;
import com.rossvideo.catena.device.impl.ConstraintUtils;
import com.rossvideo.catena.device.impl.ParamManager;
import com.rossvideo.catena.device.impl.ParamManager.WidgetHint;
import com.rossvideo.catena.example.error.UnknownOidException;
import com.rossvideo.catena.utils.IOUtils;

import catena.core.externalobject.ExternalObjectPayload;
import catena.core.externalobject.ExternalObjectRequestPayload;
import catena.core.language.PolyglotText;
import catena.core.parameter.CommandResponse;
import catena.core.parameter.DataPayload;
import catena.core.parameter.ExecuteCommandPayload;
import catena.core.parameter.Param;
import catena.core.parameter.ParamType;
import catena.core.parameter.SetValuePayload;
import catena.core.parameter.Value;
import catena.core.parameter.Empty;
import io.grpc.stub.StreamObserver;

public class MyCatenaDevice extends BasicCatenaDevice {
    
    protected static final String CATENA_ICON = "catena_icon";
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
    private CommandHandler commandHandler;

    public MyCatenaDevice(CatenaServer server, int slot, File workingDirectory) { 
        super(server, slot);
        this.workingDirectory = workingDirectory;
        this.commandHandler = new CommandHandler(workingDirectory); 
        init();
    }
    
    protected void init() {
        super.init();
        getDeviceBuilder().addAccessScopes("monitor")
        .setDefaultScope("monitor");
        buildMenus();
        buildParams();
        buildCommands();
        startTimeUpdate();
    }

    private void buildMenus() {
        getMenuManager().createMenuGroup("config", "Config")
        .getNameBuilder().putDisplayStrings("fr", "Configuration")
        .putDisplayStrings("es", "Configuración");
        getMenuManager().createMenu("config", "config", "Configuration")
        .getNameBuilder().putDisplayStrings("fr", "Configuration")
        .putDisplayStrings("es", "Configuración");;
        getMenuManager().addParamsMenu("config", "config", new String[] {FLOAT_OID, FLOAT_OID_RANGE, INT_OID, CHOICE_OID});
        getMenuManager().createMenu("config", "clock", "Clock")
            .getNameBuilder().putDisplayStrings("fr", "Horloge")
            .putDisplayStrings("es", "Reloj");
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
        Value clockOn = getParamManager().getValue(CLOCK_ON_OID);
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
            setValue(DATE_AND_TIME_OID, Value.newBuilder().setStringValue(time).build());
        }
        catch (UnknownOidException ex)
        {
            ex.printStackTrace();
        }
    }
    
    
    
    @Override
    public void setValue(SetValuePayload request, StreamObserver<Empty> responseObserver, Map<String, Object> claims)
    {
        super.setValue(request, responseObserver, claims);
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
        Param.Builder builder = manager.createParamDescriptor(CATENA_DISPLAY_NAME, "Display Name", ParamType.STRING, true, Value.newBuilder().setStringValue("Display Name").build(), null, WidgetHint.TEXT_DISPLAY)
            .addOidAliases("0xFF01");
        addParamName(builder, "fr", "Nom de l'appareil visible");
        addParamName(builder, "es", "Nombre para mostrar");
        
        builder = manager.createParamDescriptor(CATENA_PRODUCT_NAME, "Product Name", ParamType.STRING, true, Value.newBuilder().setStringValue("Example Device").build())
        .addOidAliases("0x105");
        addParamName(builder, "fr", "Nom du produit");
        addParamName(builder, "es", "Nombre del producto");

        
        builder = manager.createParamDescriptor(CATENA_ICON, "Product Icon", ParamType.STRING, true, Value.newBuilder().setStringValue("eo://icon.png").build())
            .addOidAliases("0xFF0C");
        addParamName(builder, "fr", "Icône du produit");
        addParamName(builder, "es", "Icono de producto");
        
        
        builder = manager.createParamDescriptor(STRUCT_OID, "Serial Number", ParamType.STRING, true, Value.newBuilder().setStringValue("12-345-6989").build(), null, WidgetHint.TEXT_DISPLAY);
        addParamName(builder, "fr", "Numéro de série");
        addParamName(builder, "es", "Número de serie");

        builder = manager.createParamDescriptor(DATE_AND_TIME_OID, "Date and Time", ParamType.STRING, true, Value.newBuilder().setStringValue(getTime()).build(), null, WidgetHint.LABEL);
        addParamName(builder, "fr", "Date et heure");
        addParamName(builder, "es", "Fecha y hora");
        addParamName(builder, "en_CA", "Date and Time, Eh");
        addParamName(builder, "en_GB", "Date and Time, Innit");
        builder = manager.createParamDescriptor(CLOCK_ON_OID, "Clock On", ParamType.INT32, false, Value.newBuilder().setInt32Value(0).build(), ConstraintUtils.buildIntChoiceConstraint(new String[] {
                "Off", "On"
        }), WidgetHint.CHECKBOX);
        addParamName(builder, "fr", "Horloge activée");
        addParamName(builder, "es", "Reloj activado");
        builder = manager.createParamDescriptor(FLOAT_OID, "Floating Point", ParamType.FLOAT32, false, Value.newBuilder().setFloat32Value(0f).build());
        addParamName(builder, "fr", "Nombre à virgule flottante");
        addParamName(builder, "es", "Punto flotante");
        
        builder = manager.createParamDescriptor(FLOAT_OID_RANGE, "Float Range Parameter", ParamType.FLOAT32, false, Value.newBuilder().setFloat32Value(0f).build(), ConstraintUtils.buildFloatRangeConstraint(0f, 100f, 0.1f));
        addParamName(builder, "fr", "Paramètre de plage flottante");
        addParamName(builder, "es", "Parámetro de rango de flotación");

        builder = manager.createParamDescriptor(INT_OID, "Int Parameter", ParamType.INT32, false, Value.newBuilder().setInt32Value(0).build());
        addParamName(builder, "fr", "Nombre entier");
        addParamName(builder, "es", "Entero");

        builder = manager.createParamDescriptor(CHOICE_OID, "Choice Parameter", ParamType.INT32, false, Value.newBuilder().setInt32Value(0).build(), ConstraintUtils.buildIntChoiceConstraint("en", new String[] {
                        "Choice 1", "Choice 2", "Choice 3"
                }));
        addParamName(builder, "fr", "Paramètre de choix");
        addParamName(builder, "es", "Parámetro de elección");
    }
    
    private PolyglotText.Builder addParamName(Param.Builder builder, String lang, String name)
    {
        PolyglotText.Builder text = builder.getNameBuilder();
        text.putDisplayStrings(lang, name);
        return text;
    }
    
    private void buildCommands() {
        CommandManager manager = getCommandManager();
        manager.createParamDescriptor(CMD_FOO_OID, "String Command", ParamType.STRING, false, Value.newBuilder().setStringValue("").build());
        manager.createParamDescriptor(CMD_REVERSE_OID, "Reverse String", ParamType.STRING, false, Value.newBuilder().setStringValue("").build());
        manager.createParamDescriptor(CMD_FILE_RECEIVE_OID, "File Receive", ParamType.DATA, false, Value.newBuilder().setDataPayload(DataPayload.newBuilder().build()).build());
        manager.createParamDescriptor(CMD_FILE_TRANSMIT_OID, "File Transmit", ParamType.STRING, false, Value.newBuilder().setStringValue("").build());
    }

    @Override
    public void executeCommand(ExecuteCommandPayload request, StreamObserver<CommandResponse> responseStream, Map<String, Object> claims)
    {
        int commandSlot = request.getSlot();
		String oid = request.getOid();
		boolean respond = request.getRespond();

		System.out.println("SERVER: processing command:");
		System.out.println("\tslot:    " + commandSlot);
		System.out.println("\toid:     " + oid);
		System.out.println("\trespond: " + respond);

        commandHandler.executeCommand(request, responseStream);
    }
    
    

    @Override
    public void externalObjectRequest(ExternalObjectRequestPayload request, StreamObserver<ExternalObjectPayload> responseObserver, Map<String, Object> claims)
    {
        String oid = request.getOid();
        try
        {
            if (sendResourceFromWorkingDirectory(request, responseObserver, claims, oid))
            {
                return;
            }
        }
        catch (Exception e)
        {
            responseObserver.onError(e);
            return;
        }
        
        if (sendResourceFromPackage(responseObserver, oid))
        {
            return;
        }        
        
        super.externalObjectRequest(request, responseObserver, claims);
    }

    private boolean sendResourceFromWorkingDirectory(ExternalObjectRequestPayload request, StreamObserver<ExternalObjectPayload> responseObserver, Map<String, Object> claims, String oid) throws FileNotFoundException, IOException
    {
        File workingDirectory = getWorkingDirectory();
        if (workingDirectory == null || oid == null || oid.isEmpty())
        {
            return false;
        }
        
        File requestedFile = new File(workingDirectory, oid);
        if (!requestedFile.exists() || !requestedFile.isFile() || !requestedFile.canRead())
        {
            return false;
        }
                
        try (FileInputStream fis = new FileInputStream(requestedFile))
        {
            byte[] payload = IOUtils.readAllBytes(fis);
            ExternalObjectPayload response = ExternalObjectPayload
                    .newBuilder()
                    .setPayload(
                            DataPayload.newBuilder()
                            .setPayload(ByteString.copyFrom(payload))).build();
            responseObserver.onNext(response);
            responseObserver.onCompleted();
            return true;
        }
        finally {}
    }

    private boolean sendResourceFromPackage(StreamObserver<ExternalObjectPayload> responseObserver, String oid)
    {
        try (InputStream resource = MyCatenaDevice.class.getResourceAsStream("files/" + oid)) {
            if (resource != null)
            {
                byte[] payload = IOUtils.readAllBytes(resource);
                ExternalObjectPayload response = ExternalObjectPayload.newBuilder().setPayload(DataPayload.newBuilder().setPayload(ByteString.copyFrom(payload))).build();
                responseObserver.onNext(response);
                responseObserver.onCompleted();
                return true;
            }
        }
        catch (Exception e)
        {
            responseObserver.onError(e);
        }
        
         return false;
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
