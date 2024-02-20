package com.rossvideo.catena.device.impl;

import com.rossvideo.catena.device.impl.ParamManager.WidgetHint;

import catena.core.constraint.Constraint;
import catena.core.device.Device;
import catena.core.parameter.Param.Builder;
import catena.core.parameter.ParamType;
import catena.core.parameter.Value;

public class CommandManager extends ParamManager
{

    public CommandManager(Device.Builder deviceBuilder)
    {
        super(deviceBuilder);
    }
    
    

    @Override
    public Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value, Constraint constraint, WidgetHint widget)
    {
        return super.createParamDescriptor(oid, name, type, readOnly, value, constraint, widget)
                .setResponse(true);
    }



    @Override
    protected catena.core.parameter.Param.Builder initTopLevelParam(String oid)
    {
        return getDeviceBuilder().putCommandsBuilderIfAbsent(oid);
    }
    
    public void setResponds(String oid, boolean responds)
    {
        createOrGetParam(oid).setResponse(responds);
    }

}
