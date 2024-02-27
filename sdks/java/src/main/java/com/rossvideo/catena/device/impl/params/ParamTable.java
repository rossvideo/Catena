package com.rossvideo.catena.device.impl.params;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import com.rossvideo.catena.example.error.UnknownOidException;

import catena.core.device.Device;
import catena.core.parameter.Param;
import catena.core.parameter.Value;

public class ParamTable
{
    private Map<String, ParamReference> topLevelParams = new HashMap<>();
    private Device.Builder deviceBuilder;
    
    public ParamTable() {
        
    }
    
    public void commitChanges() {
        for (Map.Entry<String, ParamReference> entry : topLevelParams.entrySet())
        {
            entry.getValue().commitValue();
            commitTopLevelParam(deviceBuilder, entry.getKey(), entry.getValue().getDefinition());
        }
    }
    
    protected void commitTopLevelParam(Device.Builder deviceBuilder, String oid, Param param)
    {
        deviceBuilder.putParams(oid, param);
    }
    
    private String[] splitOid(String fqoid)
    {
        if (fqoid.startsWith("/")) {
            fqoid = fqoid.substring(1);
        }
        
        return fqoid.split("/");
    }
    

    
    public void putParam(String oid, Param param)
    {
        if (isTopLevel(oid)) {
            createParamReference(oid, param);            
        } else {
            String[] parts = splitOid(oid);
            ParamReference top = topLevelParams.get(parts[0]);
            if (top != null)
            {
                top.putParam(parts, 1, param);
                return;
            }
            throw new UnknownOidException("No such parameter: " + parts[0]);
        }
    }
    
    public void putValue(String oid, Value value)
    {
        if (isTopLevel(oid)) {
            ParamReference ref = topLevelParams.get(oid);
            ref.initValue(value.toBuilder());
        } else {
            String[] parts = splitOid(oid);
            ParamReference top = topLevelParams.get(parts[0]);
            if (top != null)
            {
                top.putValue(parts, 1, value);
                return;
            }
            throw new UnknownOidException("No such parameter: " + parts[0]);
        }
    }
    
    public boolean isTopLevel(String oid)
    {
        return oid.lastIndexOf('/') <= 0;
    }

    public void updateFrom(Device.Builder deviceBuilder)
    {
        this.deviceBuilder = deviceBuilder;
        topLevelParams.clear();
        for (Map.Entry<String, Param> entry : getParams(deviceBuilder))
        {
            createParamReference(entry.getKey(), entry.getValue());
        }
    }
    
    protected Set<Map.Entry<String, Param>> getParams(Device.Builder builder)
    {
        return deviceBuilder.getParamsMap().entrySet();
    }
    
    private void createParamReference(String oid, Param param)
    {
        topLevelParams.put(oid, new ParamReference(oid, null, param, null, true));
    }
    
    public Param getParam(String oid)
    {
        if (isTopLevel(oid)) {
            ParamReference ref = topLevelParams.get(oid);
            if (ref != null) {
                return ref.getDefinition();
            }
        } else {
            String[] parts = splitOid(oid);
            ParamReference top = topLevelParams.get(parts[0]);
            if (top != null)
            {
                return top.getParam(parts, 1);
            }
        }
        return null;
    }
    
    public Value getValue(String oid)
    {
        if (isTopLevel(oid)) {
            ParamReference ref = topLevelParams.get(oid);
            if (ref != null) {
                return ref.getValue().build();
            }
        } else {
            String[] parts = splitOid(oid);
            ParamReference top = topLevelParams.get(parts[0]);
            if (top != null)
            {
                return top.getValue(parts, 1);
            }
        }
        return null;
    }
    
    
}
