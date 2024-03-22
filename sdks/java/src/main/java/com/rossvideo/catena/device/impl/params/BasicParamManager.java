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
            PARAM_BUILDER,
            VALUE,
            VALUE_BUILDER,
            STRUCT_VALUE,
            STRUCT_VALUE_BUILDER,
            STRUCT_FIELD,
            STRUCT_FIELD_BUILDER
        }
        
        private PathToComponent parent; 
        private Type type;
        private Param param;
        private Param.Builder paramBuilder;
        private Value value;
        private Value.Builder valueBuilder;
        private StructValue structValue;
        private StructValue.Builder structValueBuilder;
        private String fieldName;
        private StructField structField;
        private StructField.Builder structFieldBuilder;
        
        public PathToComponent(PathToComponent parent, String fieldName, Param param)
        {
            this.parent = parent;
            this.fieldName = fieldName;
            this.type = Type.PARAM;
            this.param = param;
        }
        
        public PathToComponent(PathToComponent parent, Value value)
        {
            this.parent = parent;
            this.type = Type.VALUE;
            this.value = value;
        }
        
        public PathToComponent(PathToComponent parent, StructValue structValue)
        {
            this.parent = parent;
            this.type = Type.STRUCT_VALUE;
            this.structValue = structValue;
        }
        
        public PathToComponent(PathToComponent parent, String fieldName, StructField structField)
        {
            this.parent = parent;
            this.fieldName = fieldName;
            this.type = Type.STRUCT_FIELD;
            this.structField = structField;
        }
        
        public PathToComponent(PathToComponent parent, String fieldName, Param.Builder param)
        {
            this.parent = parent;
            this.fieldName = fieldName;
            this.type = Type.PARAM_BUILDER;
            this.paramBuilder = param;
        }
        
        public PathToComponent(PathToComponent parent, Value.Builder value)
        {
            this.parent = parent;
            this.type = Type.VALUE_BUILDER;
            this.valueBuilder = value;
        }
        
        public PathToComponent(PathToComponent parent, StructValue.Builder structValue)
        {
            this.parent = parent;
            this.type = Type.STRUCT_VALUE_BUILDER;
            this.structValueBuilder = structValue;
        }
        
        public PathToComponent(PathToComponent parent, String fieldName, StructField.Builder structField)
        {
            this.parent = parent;
            this.fieldName = fieldName;
            this.type = Type.STRUCT_FIELD_BUILDER;
            this.structFieldBuilder = structField;
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
        
        public Param getParam()
        {
            return param;
        }

        public Value getValue()
        {
            return value;
        }

        public StructValue getStructValue()
        {
            return structValue;
        }

        public StructField getStructField()
        {
            return structField;
        }

        public Param.Builder getParamBuilder()
        {
            return paramBuilder;
        }
        
        public Value.Builder getValueBuilder()
        {
            return valueBuilder;
        }
        
        public StructValue.Builder getStructValueBuilder()
        {
            return structValueBuilder;
        }
        
        public StructField.Builder getStructFieldBuilder()
        {
            return structFieldBuilder;
        }
        
        public void updateParam(Param.Builder update)
        {
            if (parent != null && parent.getType() == Type.STRUCT_FIELD_BUILDER) {
                parent.getStructFieldBuilder().setParam(update);
            } else if (parent != null  && parent.getType() == Type.PARAM_BUILDER) {
                parent.getParamBuilder().putParams(fieldName, update.build());
            }
        }
        
        public void setValue(Value value)
        {
            if (type == Type.PARAM_BUILDER) {
                paramBuilder.setValue(value);
            } else if (type == Type.STRUCT_FIELD_BUILDER) {
                if (structFieldBuilder.hasParam()) {
                    structFieldBuilder.setParam(structFieldBuilder.getParam().toBuilder().setValue(value));
                } else {
                    structFieldBuilder.setValue(value);
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
                PathToComponent leaf = pathToValue(splitOid(oid), 0, null, true);
                if (leaf != null)
                {
                    leaf.setValue(uncommittedValue);
                    PathToComponent root = leaf.getRoot();
                    putTopLevelParam(root.getFieldName(), root.getParamBuilder().build());
                }
            }
            
            if (uncommittedParam != null) {
                PathToComponent leaf = pathToParam(splitOid(oid), 0, null, true);
                if (leaf != null)
                {
                    leaf.updateParam(uncommittedParam);
                    PathToComponent root = leaf.getRoot();
                    putTopLevelParam(root.getFieldName(), root.getParamBuilder().build());
                }
            }
        }
    }
    
    protected PathToComponent pathToParam(String[] oidParts, int index, PathToComponent parent, boolean mutable)
    {
        if (index == oidParts.length) {
            return parent;
        }
        
        if (index == 0) {
            if (mutable) {
                Param.Builder param = getBuilderForParam(oidParts[0], null);
                return pathToParam(oidParts, index + 1, new PathToComponent(null, oidParts[0], param), mutable);                
            } else {
                Param param = getTopLevelParam(oidParts[0]);
                if (param == null)
                {
                    return null;
                }
                return pathToParam(oidParts, index + 1, new PathToComponent(null, oidParts[0], param), mutable);
            }
        } else {
            switch (parent.getType()) {
                case PARAM_BUILDER:
                {
                    Param.Builder param = parent.getParamBuilder();
                    
                    //TODO handle getting the paramBuilder from the Value
                    Param.Builder build = getBuilderForParam(oidParts[index], param);
                    PathToComponent paramChild = new PathToComponent(parent, oidParts[index], build);
                    return pathToParam(oidParts, index + 1, paramChild, mutable);
                }
                case PARAM:
                {
                    Param param = parent.getParam();
                    Param child = param.getParamsOrDefault(oidParts[index], null);
                    if (child == null)
                    {
                        return null;
                    }
                    PathToComponent paramChild = new PathToComponent(parent, oidParts[index], child);
                    return pathToParam(oidParts, index + 1, paramChild, mutable);
                }
                default:
                    return null;
            }
        }
    }
    
    protected void putTopLevelParam(String fieldName, Param param)
    {
        deviceBuilder.putParams(fieldName, param);
    }
    
    protected Param getTopLevelParam(String fieldName) {
        return deviceBuilder.getParamsOrDefault(fieldName, null);
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
    
    protected PathToComponent pathToValue(String[] oidParts, int index, PathToComponent parent, boolean mutable)
    {
        if (index == oidParts.length) {
            return parent;
        }
        
        if (index == 0) {
            if (mutable) {
                Param.Builder param = getBuilderForParam(oidParts[0], null);
                return pathToValue(oidParts, index + 1, new PathToComponent(null, oidParts[0], param), mutable);                
            } else {
                Param param = getTopLevelParam(oidParts[0]);
                if (param == null)
                {
                    return null;
                }
                return pathToValue(oidParts, index + 1, new PathToComponent(null, oidParts[0], param), mutable);   
            }

        } else {
            switch (parent.getType()) {
                case PARAM_BUILDER:
                {
                    Param.Builder param = parent.getParamBuilder();
                    Value.Builder value = param.getValueBuilder();
                    PathToComponent valueChild = new PathToComponent(parent, value);
                    return pathToValue(oidParts, index, valueChild, mutable);
                }
                case PARAM: 
                {
                    Value value = parent.getParam().getValue();
                    if (value == null)
                    {
                        return null;
                    }
                    return pathToValue(oidParts, index, new PathToComponent(parent, value), mutable);
                }
                case VALUE_BUILDER:
                {
                    Value.Builder value = parent.getValueBuilder();
                    StructValue.Builder structValue = value.getStructValueBuilder();
                    PathToComponent structValueChild = new PathToComponent(parent, structValue);
                    StructField.Builder structFieldBuilder = getBuilderForStructField(oidParts[index], structValue);
                    PathToComponent fieldChild = new PathToComponent(structValueChild, oidParts[index], structFieldBuilder);
                    return pathToValue(oidParts, index + 1, fieldChild, mutable);
                }
                case VALUE:
                {
                    Value value = parent.getValue();
                    StructValue structValue = value.getStructValue();
                    PathToComponent structValueChild = new PathToComponent(parent, structValue);
                    StructField structField = structValue.getFieldsOrDefault(oidParts[index], null);
                    if (structField == null)
                    {
                        return null;
                    }
                    PathToComponent fieldChild = new PathToComponent(structValueChild, oidParts[index], structField);
                    return pathToValue(oidParts, index + 1, fieldChild, mutable);
                }
                case STRUCT_FIELD_BUILDER:
                {
                    StructField.Builder structField = parent.getStructFieldBuilder();
                    if (structField.hasParam()) {
                        Param.Builder structFieldValue = structField.getParamBuilder();
                        return pathToValue(oidParts, index, new PathToComponent(parent, oidParts[index], structFieldValue), mutable);
                    } else {
                        Value.Builder structFieldValue = structField.getValueBuilder();
                        return pathToValue(oidParts, index, new PathToComponent(parent, structFieldValue), mutable);
                    }
                }
                case STRUCT_FIELD:
                {
                    StructField structField = parent.getStructField();
                    if (structField.hasParam()) {
                        Param structFieldValue = structField.getParam();
                        return pathToValue(oidParts, index, new PathToComponent(parent, oidParts[index], structFieldValue), mutable);
                    } else {
                        Value structFieldValue = structField.getValue();
                        return pathToValue(oidParts, index, new PathToComponent(parent, structFieldValue), mutable);
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
        
        PathToComponent value = pathToValue(splitOid(oid), 0, null, false);
        if (value != null)
        {
            switch (value.getType())
            {
                case PARAM:
                    return value.getParam().getValue();
                case VALUE:
                    return value.getValue();
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
