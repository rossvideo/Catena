package com.rossvideo.catena.device.impl.params;

import java.util.HashMap;
import java.util.Map;

import com.rossvideo.catena.example.error.UnknownOidException;

import catena.core.parameter.Param;
import catena.core.parameter.StructField;
import catena.core.parameter.StructValue;
import catena.core.parameter.Value;

public class ParamReference
{
    private String subOid;

    private Integer index;

    private Param definition;

    private ParamReference parent;

    private Value.Builder currentValue;

    private Map<String, ParamReference> children;

    private boolean writeValueToParam;

    public ParamReference(String subOid, Integer index, Param definition, ParamReference parent, boolean writeValueToParam)
    {
        this.subOid = subOid;
        this.index = index;
        this.definition = definition;
        this.parent = parent;
        this.writeValueToParam = writeValueToParam;
        if (parent != null)
        {
            parent.registerChild(this);
        }
    }
    
    void setDefinition(Param definition, boolean writeValueToParam) {
        this.definition = definition;
        currentValue = definition.getValue().toBuilder();
        this.writeValueToParam = writeValueToParam;
        children = null;
    }
    
    public void putParam(String[] oid, int hierarchyIndex, Param param)
    {
        Integer subIndex = null;
        if (oid[hierarchyIndex].matches("\\d+"))
        {
            subIndex = Integer.parseInt(oid[hierarchyIndex]);
            hierarchyIndex++;
        }
        
        String subOid = oid[hierarchyIndex];
        if (subIndex != null) {
            subOid = subIndex + "/" + subOid;
        }
        
        if (hierarchyIndex == oid.length - 1)
        {
            if (subIndex == null && definition != null)
            {
                definition = definition.toBuilder().putParams(subOid,  param).build();
            }
            new ParamReference(subOid, subIndex, param, this, true);
        }
        else
        {
            ParamReference child = children != null ? children.get(subOid) : null;
            if (child != null) {
                child.putParam(oid, hierarchyIndex+1, param);
            } else {
                throw new UnknownOidException("No such parameter: " + getOid() + "/" + subOid);
            }
            
        }
    }
    
    private Param getSubParamDefinition(String subOid, StructField field) {
        if (field != null && field.hasParam()) {
            return field.getParam();
        } else {
            return definition.getParamsOrThrow(subOid);
        }
    }
    
    public void initValue(Value.Builder value)
    {
        currentValue = value;
        children = null;
        if (value.hasStructValue())
        {
            StructValue structValue = value.getStructValue();
            for (Map.Entry<String, StructField> field : structValue.getFieldsMap().entrySet())
            {
                ParamReference child = new ParamReference(field.getKey(), null, getSubParamDefinition(field.getKey(), field.getValue()), this, field.getValue().hasParam());
                if (field.getValue().hasParam()) {
                    child.initValue(field.getValue().getParam().getValue().toBuilder());                    
                }
                else if (field.getValue().hasValue()) {
                    child.initValue(field.getValue().getValue().toBuilder());
                }
            }
        }
    }

    public void registerChild(ParamReference child)
    {
        if (children == null)
        {
            children = new HashMap<>();
        }
        children.put(child.getIndexedSubOid(), child);
    }

    public String getOid()
    {
        if (parent == null)
        {
            return subOid;
        }
        else if (index != null)
        {
            return parent.getOid() + "/" + index + "/" + subOid;
        }
        else
        {
            return parent.getOid() + "/" + subOid;
        }
    }
    
    public String createIndexedSubOid(String subOid, Integer index)
    {
        if (index != null)
        {
            return index + "/" + subOid;
        }
        return subOid;
    }
    
    public String getIndexedSubOid() {
        return createIndexedSubOid(subOid, index);
    }

    public String getSubOid()
    {
        return subOid;
    }

    public Integer getIndex()
    {
        return index;
    }
    
    public boolean hasChildren()
    {
        return children != null && !children.isEmpty();
    }
    
    public Value.Builder getValue()
    {
        return currentValue;
    }

    public Param getDefinition()
    {
        return definition;
    }

    public void commitValue()
    {
        if (hasChildren())
        {
            for (Map.Entry<String, ParamReference> child : children.entrySet())
            {
                updateSubValue(child.getValue());
            }
        }
        
        if (writeValueToParam && currentValue != null)
        {
            definition = definition.toBuilder().setValue(currentValue).build();
        }
    }

    protected void updateSubValue(ParamReference paramReference)
    {
        paramReference.commitValue();
        switch (definition.getType())
        {
            case STRUCT:
                updateStructValue(paramReference);
                break;
            case STRUCT_ARRAY:
            case STRUCT_VARIANT:
            case STRUCT_VARIANT_ARRAY:
                break;
            default:
                throw new IllegalArgumentException("Parameter is not a struct: " + getOid());
        }
    }

    private void updateStructValue(ParamReference paramReference)
    {
        if (currentValue == null)
        {
            currentValue = Value.newBuilder();
        }

        StructValue.Builder structValueBuilder = null;
        if (currentValue.hasStructValue())
        {
            structValueBuilder = currentValue.getStructValueBuilder();
        }
        else
        {
            structValueBuilder = StructValue.newBuilder();
            currentValue.setStructValue(structValueBuilder);
        }

        StructField existingField = structValueBuilder.getFieldsOrDefault(paramReference.getSubOid(), null);
        StructField.Builder existingFieldBuilder = null;
        if (existingField == null)
        {
            existingFieldBuilder = StructField.newBuilder();
        }
        else
        {
            existingFieldBuilder = existingField.toBuilder();
        }

        if (existingFieldBuilder.hasParam())
        {
            existingFieldBuilder.setParam(paramReference.getDefinition());
        }
        else if (paramReference.getValue() != null)
        {
            existingFieldBuilder.setValue(paramReference.getValue());
        }

        structValueBuilder.putFields(paramReference.getSubOid(), existingFieldBuilder.build());

        currentValue.setStructValue(structValueBuilder);
    }

    ParamReference getParamRef(String[] oid, int hierarchyIndex)
    {
        if (!hasChildren())
        {
            return null;
        }
        
        Integer subIndex = null;
        if (oid[hierarchyIndex].matches("\\d+"))
        {
            subIndex = Integer.parseInt(oid[hierarchyIndex]);
            hierarchyIndex++;
        }
        
        String subOid = oid[hierarchyIndex];
        if (subIndex != null) {
            subOid = subIndex + "/" + subOid;
        }
        
        if (hierarchyIndex == oid.length - 1)
        {
            ParamReference child = children.get(subOid);
            if (child != null)
            {
                return child;
            }
            return null;
        }
        else
        {
            ParamReference child = children.get(subOid);
            if (child != null)
            {
                return child.getParamRef(oid, hierarchyIndex++);
            }
            return null;
        }
    }
    
    public Value getValue(String[] oid, int hierarchyIndex)
    {
        ParamReference ref = getParamRef(oid, hierarchyIndex);
        if (ref != null)
        {
            Value.Builder builder = ref.getValue();
            if (builder != null)
            {
                return builder.build();
            }
        }
        return null;
    }
    
    public Param getParam(String[] oid, int hierarchyIndex)
    {
        ParamReference ref = getParamRef(oid, hierarchyIndex);
        if (ref != null)
        {
            Param definition = ref.getDefinition();
            if (definition != null)
            {
                return definition;
            }
        }
        return null;
    }
    
    public void putValue(String[] oid, int hierarchyIndex, Value value)
    {
        Integer subIndex = null;
        if (oid[hierarchyIndex].matches("\\d+"))
        {
            subIndex = Integer.parseInt(oid[hierarchyIndex]);
            hierarchyIndex++;
        }
        
        String subOid = oid[hierarchyIndex];
        if (subIndex != null) {
            subOid = subIndex + "/" + subOid;
        }
        
        if (hierarchyIndex == oid.length - 1)
        {
            ParamReference child = children.get(subOid);
            if (child != null)
            {
                child.initValue(value.toBuilder());
                return;
            }
            else
            {
                child = new ParamReference(subOid, subIndex, getSubParamDefinition(oid[hierarchyIndex], null), this, false);
                child.initValue(value.toBuilder());
            }
        }
        else
        {
            ParamReference child = children.get(subOid);
            if (child != null) {
                child.putValue(oid, hierarchyIndex+1, value);
            } else {
                throw new UnknownOidException("No such parameter: " + getOid() + "/" + subOid);
            }
            
        }
    }

    @Override
    public String toString()
    {
        return "ParamReference [subOid=" + subOid + ", index=" + index + ", definition=" + definition + ", currentValue=" + currentValue + "]";
    }
    
    
}
