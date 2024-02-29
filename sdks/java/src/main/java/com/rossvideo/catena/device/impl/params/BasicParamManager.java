package com.rossvideo.catena.device.impl.params;

import java.util.HashMap;
import java.util.Map;

import com.rossvideo.catena.device.impl.ParamManager;
import com.rossvideo.catena.device.impl.TextUtils;

import catena.core.constraint.Constraint;
import catena.core.device.Device;
import catena.core.parameter.Param;
import catena.core.parameter.Param.Builder;
import catena.core.parameter.ParamType;
import catena.core.parameter.StructField;
import catena.core.parameter.StructValue;
import catena.core.parameter.Value;

public class BasicParamManager implements ParamManager
{
    protected static class PathToComponent
    {
        public enum Type {
            PARAM,
            VALUE,
            STRUCT_VALUE,
            STRUCT_FIELD
        }
        
        private PathToComponent parent; 
        private Type type;
        private Param.Builder param;
        private Value.Builder value;
        private StructValue.Builder structValue;
        private String fieldName;
        private StructField.Builder structField;
        
        public PathToComponent(PathToComponent parent, String fieldName, Param.Builder param)
        {
            this.parent = parent;
            this.fieldName = fieldName;
            this.type = Type.PARAM;
            this.param = param;
        }
        
        public PathToComponent(PathToComponent parent, Value.Builder value)
        {
            this.parent = parent;
            this.type = Type.VALUE;
            this.value = value;
        }
        
        public PathToComponent(PathToComponent parent, StructValue.Builder structValue)
        {
            this.parent = parent;
            this.type = Type.STRUCT_VALUE;
            this.structValue = structValue;
        }
        
        public PathToComponent(PathToComponent parent, String fieldName, StructField.Builder structField)
        {
            this.parent = parent;
            this.fieldName = fieldName;
            this.type = Type.STRUCT_FIELD;
            this.structField = structField;
        }
        
        public String getFieldName()
        {
            return fieldName;
        }
        
        public PathToComponent getRoot()
        {
            if (parent != null) {
                return parent.getRoot();
            }
            return this;
        }
        
        /*
        public void commit() 
        {
            PathToComponent parent = getParent();
            while (parent != null && parent.getParent() != null) {
                parent = parent.getParent();
            }
            
            if (parent != null) {
                parent.commit();
            }
        }*/
        
        public Type getType()
        {
            return type;
        }
        
        public PathToComponent getParent()
        {
            return parent;
        }
        
        public Param.Builder getParam()
        {
            return param;
        }
        
        public Value.Builder getValue()
        {
            return value;
        }
        
        public StructValue.Builder getStructValue()
        {
            return structValue;
        }
        
        public StructField.Builder getStructField()
        {
            return structField;
        }
        
        public void updateParam(Param.Builder update)
        {
            if (parent != null && parent.getType() == Type.STRUCT_FIELD) {
                parent.getStructField().setParam(update);
            } else if (parent != null  && parent.getType() == Type.PARAM) {
                parent.getParam().putParams(fieldName, update.build());
            }
        }
        
        public void setValue(Value value)
        {
            if (type == Type.PARAM) {
                param.setValue(value);
            } else if (type == Type.STRUCT_FIELD) {
                if (structField.hasParam()) {
                    structField.setParam(structField.getParam().toBuilder().setValue(value));
                } else {
                    structField.setValue(value);
                }
            }
        }
    }
    
    private Device.Builder deviceBuilder;
    private Map<String, Param.Builder> uncommittedParams = new HashMap<>();
    private Map<String, Value> uncommittedValues = new HashMap<>();
    
    public BasicParamManager(Device.Builder deviceBuilder)
    {
        this.deviceBuilder = deviceBuilder;
    }
    
    protected Device.Builder getDeviceBuilder() {
        return deviceBuilder;
    }
    
    private String[] splitOid(String fqoid)
    {
        if (fqoid.startsWith("/")) {
            fqoid = fqoid.substring(1);
        }
        
        return fqoid.split("/");
    }
    
    private boolean isTopLevel(String oid)
    {
        return oid.lastIndexOf('/') <= 0;
    }
    
    protected Builder createOrGetParam(String[] oidParts, int index, Param.Builder parent)
    {
        if (parent.getType() != ParamType.STRUCT && parent.getType() != ParamType.STRUCT_ARRAY) {
            parent.setType(ParamType.STRUCT);
        }
        
        Param.Builder existingBuilder = this.getBuilderForParam(oidParts[index], parent);
        if (index < oidParts.length - 1)
        {
            return createOrGetParam(oidParts, index + 1, existingBuilder);
        }
        
        return existingBuilder;
    }
    
    @Override
    public Builder createOrGetParam(String oid)
    {
        if (isTopLevel(oid)) {
            return getBuilderForParam(oid, null);
        } else {
            String[] parts = splitOid(oid);
            return createOrGetParam(parts, 1, createOrGetParam(parts[0]));
        }
    }

    @Override
    public void clearParams()
    {
        // TODO Auto-generated method stub

    }

    @Override
    public void commitChanges()
    {
        for (Object key : uncommittedParams.keySet().toArray())
        {
            commitChanges(key.toString());
        }
        for (Object key : uncommittedValues.keySet().toArray())
        {
            commitChanges(key.toString());
        }
    }

    @Override
    public void commitChanges(String oid)
    {
        Param.Builder uncommittedParam = uncommittedParams.remove(oid);
        Value uncommittedValue = uncommittedValues.remove(oid);
        if (uncommittedParam == null && uncommittedValue == null)
        {
            return;
        }
        
        if (isTopLevel(oid))
        {
            if (uncommittedParam == null)
            {
                uncommittedParam = getBuilderForParam(oid, null);
            }
            if (uncommittedValue != null)
            {
                uncommittedParam.setValue(uncommittedValue);
            }
            putTopLevelParam(oid, uncommittedParam.build());
        } else {
            if (uncommittedValue != null)
            {
                PathToComponent leaf = collectPathToValue(splitOid(oid), 0, null);
                if (leaf != null)
                {
                    leaf.setValue(uncommittedValue);
                    PathToComponent root = leaf.getRoot();
                    putTopLevelParam(root.getFieldName(), root.getParam().build());
                }
            }
            
            if (uncommittedParam != null) {
                PathToComponent leaf = collectPathToParam(splitOid(oid), 0, null);
                if (leaf != null)
                {
                    leaf.updateParam(uncommittedParam);
                    PathToComponent root = leaf.getRoot();
                    putTopLevelParam(root.getFieldName(), root.getParam().build());
                }
            }
        }
    }
    
    protected PathToComponent collectPathToParam(String[] oidParts, int index, PathToComponent parent)
    {
        if (index == oidParts.length) {
            return parent;
        }
        
        if (index == 0) {
            Param.Builder param = getBuilderForParam(oidParts[0], null);
            return collectPathToParam(oidParts, index + 1, new PathToComponent(null, oidParts[0], param));
        } else {
            switch (parent.getType()) {
                case PARAM:
                {
                    Param.Builder param = parent.getParam();
                    
                    //TODO handle getting the param from the Value
                    Param.Builder build = getBuilderForParam(oidParts[index], param);
                    PathToComponent paramChild = new PathToComponent(parent, oidParts[index], build);
                    return collectPathToParam(oidParts, index + 1, paramChild);
                }/*
                case VALUE:
                {
                    Value.Builder value = parent.getValue();
                    StructValue.Builder structValue = value.getStructValueBuilder();
                    PathToComponent structValueChild = new PathToComponent(parent, structValue);
                    StructField structField = structValue.getFieldsMap().get(oidParts[index]);
                    StructField.Builder structFieldBuilder = null;
                    if (structField == null) {
                        structFieldBuilder = StructField.newBuilder();
                        structValue.putFields(oidParts[index], structFieldBuilder.build());
                    } else
                    {
                        structFieldBuilder = structField.toBuilder();
                    }
                    
                    PathToComponent fieldChild = new PathToComponent(structValueChild, oidParts[index], structFieldBuilder);
                    return collectPathToValue(oidParts, index, fieldChild);
                }
                case STRUCT_FIELD:
                {
                    StructField.Builder structField = parent.getStructField();
                    if (structField.hasParam()) {
                        Param.Builder structFieldValue = structField.getParamBuilder();
                        return collectPathToValue(oidParts, index + 1, new PathToComponent(parent, structFieldValue));
                    } else if (structField.hasValue()) {
                        Value.Builder structFieldValue = structField.getValueBuilder();
                        return collectPathToValue(oidParts, index + 1, new PathToComponent(parent, structFieldValue));
                    }
                }*/
                default:
                    return null;
            }
        }
    }
    
    protected void putTopLevelParam(String fieldName, Param param)
    {
        deviceBuilder.putParams(fieldName, param);
    }
    
    protected Param.Builder getBuilderForTopLevelParam(String fieldName)
    {
        Param existing = deviceBuilder.getParamsOrDefault(fieldName, null);
        deviceBuilder.removeParams(fieldName);
        Param.Builder structParamBuilder = deviceBuilder.putParamsBuilderIfAbsent(fieldName);
        if (existing != null) {
            structParamBuilder.mergeFrom(existing);                        
        }
        
        return structParamBuilder;
    }
    
    protected Param.Builder getBuilderForParam(String fieldName, Param.Builder parentParam)
    {
        if (parentParam == null) {
            return getBuilderForTopLevelParam(fieldName);
        }
        Param existing = parentParam.getParamsOrDefault(fieldName, null);
        parentParam.removeParams(fieldName);
        Param.Builder structParamBuilder = parentParam.putParamsBuilderIfAbsent(fieldName);

        if (existing != null) {
            structParamBuilder.mergeFrom(existing);                        
        }
        
        return structParamBuilder;
    }
    
    protected StructField.Builder getBuilderForStructField(String fieldName, StructValue.Builder structValue)
    {
        StructField existing = structValue.getFieldsOrDefault(fieldName, null);
        structValue.removeFields(fieldName);
        StructField.Builder structFieldBuilder = structValue.putFieldsBuilderIfAbsent(fieldName);
        if (existing != null) {
            structFieldBuilder.mergeFrom(existing);                        
        }
        
        return structFieldBuilder;
    }
    
    protected PathToComponent collectPathToValue(String[] oidParts, int index, PathToComponent parent)
    {
        if (index == oidParts.length) {
            return parent;
        }
        
        if (index == 0) {
            Param.Builder param = getBuilderForParam(oidParts[0], null);
            return collectPathToValue(oidParts, index + 1, new PathToComponent(null, oidParts[0], param));
        } else {
            switch (parent.getType()) {
                case PARAM:
                {
                    Param.Builder param = parent.getParam();
                    Value.Builder value = param.getValueBuilder();
                    PathToComponent valueChild = new PathToComponent(parent, value);
                    return collectPathToValue(oidParts, index, valueChild);
                }
                case VALUE:
                {
                    Value.Builder value = parent.getValue();
                    StructValue.Builder structValue = value.getStructValueBuilder();
                    PathToComponent structValueChild = new PathToComponent(parent, structValue);
                    StructField.Builder structFieldBuilder = getBuilderForStructField(oidParts[index], structValue);
                    PathToComponent fieldChild = new PathToComponent(structValueChild, oidParts[index], structFieldBuilder);
                    return collectPathToValue(oidParts, index + 1, fieldChild);
                }
                case STRUCT_FIELD:
                {
                    StructField.Builder structField = parent.getStructField();
                    if (structField.hasParam()) {
                        Param.Builder structFieldValue = structField.getParamBuilder();
                        return collectPathToValue(oidParts, index, new PathToComponent(parent, oidParts[index], structFieldValue));
                    } else {
                        Value.Builder structFieldValue = structField.getValueBuilder();
                        return collectPathToValue(oidParts, index, new PathToComponent(parent, structFieldValue));
                    }
                }
                default:
                    return null;
            }
        }
    }

    @Override
    public Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value, Constraint constraint, WidgetHint widget)
    {
        Param.Builder builder = createOrGetParam(oid);
        builder.setName(TextUtils.createSimpleText(name))
                .setType(type)
                .setReadOnly(readOnly)
                .setValue(value);
        
        if (constraint != null) {
            builder.setConstraint(constraint);
        }
                
        setWidgetHint(builder, widget);
        uncommittedParams.put(oid, builder);
        return builder;
    }

    @Override
    public Builder setWidgetHint(Builder param, WidgetHint widgetHint)
    {
        if (widgetHint == null)
        {
            widgetHint = WidgetHint.DEFAULT;
        }
        return setWidgetHint(param, widgetHint.name().toLowerCase());
    }

    @Override
    public Builder setWidgetHint(Builder param, String widgetHint)
    {
        if (!WidgetHint.DEFAULT.name().toLowerCase().equals(widgetHint)) {
            param.setWidget(widgetHint);
        }
        else
        {
            param.clearWidget();
        }
        return param;
    }

    @Override
    public Value getValue(String oid, Integer index)
    {
      //TODO handle arrays and inefficient creation of values
        Value v = uncommittedValues.get(oid);
        if (v != null)
        {
            return v;
        }
        
        PathToComponent value = collectPathToValue(splitOid(oid), 0, null);
        if (value != null)
        {
            switch (value.getType())
            {
                case PARAM:
                    return value.getParam().getValue();
                case VALUE:
                    return value.getValue().build();
                case STRUCT_FIELD:
                    if (value.getStructField().hasParam()) {
                        return value.getStructField().getParam().getValue();
                    } else {
                        return value.getStructField().getValue();                        
                    }
                default:
                    return null;
            }
        }
        
        return null;
    }

    @Override
    public void setValue(String oid, Integer index, Value value)
    {
        //TODO handle arrays
        uncommittedValues.put(oid, value);
    }

}
