package com.rossvideo.catena.device.impl;

import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.Map;

import com.rossvideo.catena.example.error.UnknownOidException;

import catena.core.constraint.Constraint;
import catena.core.device.Device;
import catena.core.parameter.Param;
import catena.core.parameter.ParamType;
import catena.core.parameter.Value;

public class ParamManager
{
    public enum WidgetHint {
        DEFAULT,
        BUTTON,
        TEXT_DISPLAY,
        TEXT_ENTRY,
        HIDDEN,
        LABEL,
        TOGGLE,
        RADIO_BUTTONS,
        CHECKBOX,
        COLOR_CHOOSER,
        DOT,
        COMBO_BOX,
        LIST,
        DATE_PICKER,
        EQ_GRAPH,
        LINE_GRAPH,
        MULTI_LINE_TEXT,
        PASSWORD,
        SLIDER,
        FADER,
        WHEEL,
        SPINNER,
        TREE,
        JOYSTICK,
        PROGRESS,
        TABLE,
        FILE_CHOOSER        
    }
    
    private Device.Builder deviceBuilder;
    private Map<String, Param.Builder> params = new LinkedHashMap<>();
    
    public ParamManager(Device.Builder deviceBuilder)
    {
        this.deviceBuilder = deviceBuilder;
    }
    
    protected String validateOid(String oid, boolean topLevel)
    {
        if (oid == null || oid.isEmpty())
        {
            throw new IllegalArgumentException("OID cannot be null or empty");
        }
        
        if (topLevel && oid.indexOf('/') > 0) {
            throw new IllegalArgumentException("Top-level OIDs cannot contain '/' after the first character: " + oid);
        }
        
        if (oid.startsWith("/")) {
            return oid.substring(1);
        }
        
        return oid;
    }
    
    protected Device.Builder getDeviceBuilder()
    {
        return deviceBuilder;
    }
    
    protected Param.Builder getParam(String oid) {
        return params.get(oid);
    }
    
    protected Param.Builder initTopLevelParam(String oid) {
        return deviceBuilder.putParamsBuilderIfAbsent(oid);
    }
        
    protected Param.Builder createOrGetTopLevelParam(String oid)
    {
        validateOid(oid, true);
        Param.Builder paramBuilder = getParam(oid);
        if (paramBuilder == null)
        {
            paramBuilder = initTopLevelParam(oid);
            params.put(oid, paramBuilder);
        }
        
        return paramBuilder;
    }
    
    public Param.Builder createOrGetParam(String oid)
    {
        oid = validateOid(oid, true);
        
        Param.Builder paramBuilder = getParam(oid);
        if (paramBuilder != null)
        {
            return paramBuilder;
        }
        
        if (oid.lastIndexOf('/') <= 0)
        {
            return createOrGetTopLevelParam(oid);
        }
        
        String[] parts = removeTail(oid);
        Param.Builder parent = createOrGetParam(parts[0]);
        paramBuilder = parent.putParamsBuilderIfAbsent(parts[1]);
        params.put(oid, paramBuilder);
        
        return paramBuilder;
    }
    
    private String[] removeTail(String fqoid)
    {
        int index = fqoid.lastIndexOf('/');
        String[] parts = new String[2];
        parts[0] = fqoid.substring(0, index);
        parts[1] = fqoid.substring(index + 1);
        return parts;
    }
    
    public void clearParams()
    {
        deviceBuilder.clearParams();
        params.clear();
    }
    
    public Param.Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value) {
        return createParamDescriptor(oid, name, type, readOnly, value, null);
    }
    
    public Param.Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value, Constraint constraint) {
        return createParamDescriptor(oid, name, type, readOnly, value, constraint, WidgetHint.DEFAULT);
    }
    
    public Param.Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value, Constraint constraint, WidgetHint widget) {
        Param.Builder param = createOrGetParam(oid)
                .setName(TextUtils.createSimpleText(name))
                .setType(type)
                .setReadOnly(readOnly)
                .setValue(value);
        
        if (constraint != null)
        {
            param.setConstraint(constraint);
        }
        
        setWidgetHint(param, widget);
        
        return param;
    }
    
    public void addParamAlias(String fqoid, String alias) {
        createOrGetParam(fqoid).addOidAliases(alias);
    }

    public void addParamAliases(String fqoid, String[] aliases) {
        createOrGetParam(fqoid).addAllOidAliases(Arrays.asList(aliases));
    }
    
    public Param.Builder setWidgetHint(Param.Builder param, WidgetHint widgetHint) {
        if (widgetHint == null)
        {
            widgetHint = WidgetHint.DEFAULT;
        }
        return setWidgetHint(param, widgetHint.name());
    }
    
    public Param.Builder setWidgetHint(Param.Builder param, String widgetHint) {
        if (!WidgetHint.DEFAULT.name().equals(widgetHint)) {
            param.setWidget(widgetHint);
        }
        else
        {
            param.clearWidget();
        }
        return param;
    }
    
    public Value getValue(String oid) {
        Param.Builder param = getParam(oid);
        if (param != null)
        {
            return param.getValue();
        }
        throw new UnknownOidException("No such parameter: " + oid);        
    }
    
    public void setValue(String oid, Value value) {
        Param.Builder param = getParam(oid);
        if (param != null && value != null)
        {
            param.setValue(value);
        }
        else
        {
            throw new UnknownOidException("No such parameter: " + oid);
        }
    }
}
