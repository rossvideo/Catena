package com.rossvideo.catena.device;

import java.util.Map;

import com.rossvideo.catena.device.impl.CommandManager;
import com.rossvideo.catena.device.impl.MenuGroupManager;
import com.rossvideo.catena.device.impl.ParamManager;
import com.rossvideo.catena.device.impl.params.BasicCommandManager;
import com.rossvideo.catena.device.impl.params.BasicParamManager;
import com.rossvideo.catena.oauth.ScopeValidator;
import com.rossvideo.catena.oauth.SimpleScopeValidator;

import catena.core.device.Device;
import catena.core.device.DeviceComponent;
import catena.core.device.DeviceRequestPayload;
import catena.core.device.PushUpdates;
import catena.core.device.PushUpdates.PushValue;
import catena.core.parameter.GetValuePayload;
import catena.core.parameter.Param;
import catena.core.parameter.SetValuePayload;
import catena.core.parameter.Value;
import catena.core.parameter.Empty;
import io.grpc.stub.StreamObserver;

public class BasicCatenaDevice implements CatenaDevice
{
    private int slot;

    private CatenaServer server;

    private Device.Builder deviceBuilder;

    private MenuGroupManager menuGroups;

    private ParamManager paramManager;
    private CommandManager commandManager;

    public BasicCatenaDevice(CatenaServer server) {
        this(server, 0);
    }
    
    public BasicCatenaDevice(CatenaServer server, int slot) {
        this.server = server;
        this.slot = slot;
    }
    
    public void setSlot(int slot) {
        this.slot = slot;
    }

    protected CatenaServer getServer() {
        return server;
    }
    
    protected void init() {
        deviceBuilder = createDeviceBuilder();
        menuGroups = createMenuGroupManager(deviceBuilder);
        paramManager = createParamManager(deviceBuilder);
        commandManager = createCommandManager(deviceBuilder);        
    }
    
    
    protected Device.Builder createDeviceBuilder() {
        return Device.newBuilder().setSlot(getSlot());
    }

    protected MenuGroupManager createMenuGroupManager(Device.Builder deviceBuilder) {
        return new MenuGroupManager(deviceBuilder);
    }

    protected ParamManager createParamManager(Device.Builder deviceBuilder) {
        return new BasicParamManager(deviceBuilder);
    }
    
    protected CommandManager createCommandManager(Device.Builder deviceBuilder) {
        return new BasicCommandManager(deviceBuilder);
    }

    public int getSlot() {
        return slot;
    }

    public void start() {
        server.addDevice(slot, this);
    }

    public void stop() {
        server.removeDevice(slot);
    }

    @Override
    public void deviceRequest(DeviceRequestPayload request, StreamObserver<DeviceComponent> responseObserver, Map<String, Object> claims) {
        Device device = buildDeviceMessage(request);
        DeviceComponent component = DeviceComponent.newBuilder().setDevice(device).build();
        responseObserver.onNext(component);
        responseObserver.onCompleted();
    }
    
    protected Device.Builder getDeviceBuilder() {
        return deviceBuilder;
    }

    protected Device buildDeviceMessage(DeviceRequestPayload request) {
        paramManager.commitChanges();
        commandManager.commitChanges();
        return deviceBuilder.build();
    };

    protected MenuGroupManager getMenuManager() {
        return menuGroups;
    }

    protected ParamManager getParamManager() {
        return paramManager;
    }
    
    protected CommandManager getCommandManager() {
        return commandManager;
    }
    
    protected void validate(String oid, Value value) {
        
    }

    public void setValue(String oid, int index, Value value)  {
        setValue(oid, index, value, null);
    }
    
    public void setValue(String oid, int index, Value value, StreamObserver<Empty> responseObserver) {
        try 
        {
            getParamManager().setValue(oid, index, value);
            
            if (responseObserver != null)
            {
                responseObserver.onNext(Empty.getDefaultInstance());
                responseObserver.onCompleted();
            }
        }
        catch (Exception ex)
        {
            if (responseObserver != null)
            {
                responseObserver.onError(ex);
            }
        }
        
        this.getServer().notifyClients(PushUpdates.newBuilder()
                .setSlot(slot)
                .setValue(PushValue.newBuilder()
                        .setOid(oid)
                        .setElementIndex(index)
                        .setValue(value)
                        .build())
                .build());
    }
    
    @Override
    public void setValue(SetValuePayload request, StreamObserver<Empty> responseObserver, Map<String, Object> claims) {
        if (claims != null) {
            String scope = getScope(request.getOid());
            if (scope != null && !scope.isEmpty()) {
                ScopeValidator scopes = getScopes(claims);
                if (scopes == null || !scopes.isWritable(scope))
                {
                    responseObserver.onError(new IllegalArgumentException("Not authorized to write to " + request.getOid()));
                    return;
                }
            }
        }
        
        String oid = request.getOid();
        int index = request.getElementIndex();
        Value value = request.getValue();
        setValue(oid, index, value, responseObserver);
    }
    
    protected ScopeValidator getScopes(Map<String, Object> claims)
    {
        if (claims != null)
        {
            Object scopes = claims.get("scope");
            return SimpleScopeValidator.getFrom(scopes == null ? null : scopes.toString()); 
        }
        return null;
    }
    
    protected String getScope(String oid) {
        Param.Builder param = getParamManager().getParam(oid);
        if (param == null) {
            return null;
        }
        
        String scope = param.getAccessScope();
        if (scope != null && !scope.isEmpty())
        {
            return scope;
        }
        
        return deviceBuilder.getDefaultScope();
    }

    @Override
    public void getValue(GetValuePayload request, StreamObserver<Value> responseObserver, Map<String, Object> claims) {
        try
        {
            Value paramValue = getParamManager().getValue(request.getOid(), request.getElementIndex());
            responseObserver.onNext(paramValue);
            responseObserver.onCompleted();
        }
        catch (Exception ex)
        {
            responseObserver.onError(ex);
        }
    }
    
    
}
