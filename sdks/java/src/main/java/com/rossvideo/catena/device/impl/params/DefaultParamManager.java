package com.rossvideo.catena.device.impl.params;

import java.util.Arrays;

import com.rossvideo.catena.device.impl.ParamManager;
import com.rossvideo.catena.device.impl.TextUtils;
import com.rossvideo.catena.example.error.UnknownOidException;

import catena.core.constraint.Constraint;
import catena.core.device.Device;
import catena.core.parameter.Param;
import catena.core.parameter.ParamType;
import catena.core.parameter.StructField;
import catena.core.parameter.StructValue;
import catena.core.parameter.Value;

public class DefaultParamManager implements ParamManager
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
    private ParamTable paramTable;
    
    public DefaultParamManager(Device.Builder deviceBuilder)
    {
        this.deviceBuilder = deviceBuilder;
        paramTable = createParamTable();
        paramTable.updateFrom(deviceBuilder);
    }
    
    protected ParamTable createParamTable()
    {
        return new ParamTable();
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
    
    protected Param.Builder initTopLevelParam(String oid) {
        Param.Builder builder = Param.newBuilder();
        paramTable.putParam(oid, Param.newBuilder().build());
        return builder;
    }
    
    @Override
    public Param.Builder createOrGetParam(String oid)
    {
        oid = validateOid(oid, false);
        
        Param existing = paramTable.getParam(oid);
        if (existing != null) {
            return existing.toBuilder();
        }

        if (isTopLevel(oid))
        {
            return initTopLevelParam(oid);
        }
        
        String[] parts = removeTail(oid);
        Param.Builder parent = createOrGetParam(parts[0]);
        if (parent.getType() != ParamType.STRUCT || parent.getType() != ParamType.STRUCT_ARRAY)
        {
            parent.setType(ParamType.STRUCT);
            paramTable.putParam(parts[0], parent.build());
        }

        Param.Builder newParam = Param.newBuilder();
        paramTable.putParam(oid, newParam.build());
        return newParam;
    }
    
    public boolean isTopLevel(String oid)
    {
        return oid.lastIndexOf('/') <= 0;
    }
    
    private String[] removeTail(String fqoid)
    {
        int index = fqoid.lastIndexOf('/');
        String[] parts = new String[2];
        parts[0] = fqoid.substring(0, index);
        parts[1] = fqoid.substring(index + 1);
        return parts;
    }
    
    @Override
    public void clearParams()
    {
        deviceBuilder.clearParams();
        paramTable.updateFrom(deviceBuilder);
    }
    
    @Override
    public void commitChanges()
    {
        paramTable.commitChanges();
    }
    
    @Override
    public Param.Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value) {
        return createParamDescriptor(oid, name, type, readOnly, value, null);
    }
    
    @Override
    public Param.Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value, Constraint constraint) {
        return createParamDescriptor(oid, name, type, readOnly, value, constraint, WidgetHint.DEFAULT);
    }
    
    @Override
    public Param.Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value, Constraint constraint, WidgetHint widget) {
        Param.Builder param = createOrGetParam(oid)
                .setName(TextUtils.createSimpleText(name))
                .setType(type)
                .setReadOnly(readOnly)
                .setValue(value);
        
        if (!isTopLevel(oid))
        {
            setValue(oid, 0, value);
        }
        
        if (constraint != null)
        {
            param.setConstraint(constraint);
        }
        
        setWidgetHint(param, widget);
        
        paramTable.putParam(oid, param.build());
        paramTable.putValue(oid, value);
        
        return param;
    }
    
    @Override
    public void addParamAlias(String fqoid, String alias) {
        paramTable.putParam(fqoid, createOrGetParam(fqoid).addOidAliases(alias).build());
    }

    @Override
    public void addParamAliases(String fqoid, String[] aliases) {
        paramTable.putParam(fqoid, createOrGetParam(fqoid).addAllOidAliases(Arrays.asList(aliases)).build());
    }
    
    @Override
    public Param.Builder setWidgetHint(Param.Builder param, WidgetHint widgetHint) {
        if (widgetHint == null)
        {
            widgetHint = WidgetHint.DEFAULT;
        }
        return setWidgetHint(param, widgetHint.name());
    }
    
    @Override
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
    
    @Override
    public Value getValue(String oid, Integer index) {
        oid = validateOid(oid, false);
        //TODO handle index
        return paramTable.getValue(oid);   
    }
    
    @Override
    public void setValue(String oid, Integer index, Value value) {
        oid = validateOid(oid, false);
        paramTable.putValue(oid, value);
    }
}
