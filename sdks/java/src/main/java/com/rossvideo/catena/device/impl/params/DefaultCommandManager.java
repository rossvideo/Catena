package com.rossvideo.catena.device.impl.params;

import com.rossvideo.catena.device.impl.CommandManager;

import catena.core.constraint.Constraint;
import catena.core.device.Device;
import catena.core.parameter.Param.Builder;
import catena.core.parameter.ParamType;
import catena.core.parameter.Value;

public class DefaultCommandManager extends DefaultParamManager implements CommandManager
{

    public DefaultCommandManager(Device.Builder deviceBuilder)
    {
        super(deviceBuilder);
    }

    @Override
    protected ParamTable createParamTable()
    {
        return new CommandTable();
    }

    @Override
    public Builder createParamDescriptor(String oid, String name, ParamType type, boolean readOnly, Value value, Constraint constraint, WidgetHint widget)
    {
        return super.createParamDescriptor(oid, name, type, readOnly, value, constraint, widget)
                .setResponse(true);
    }

    
    @Override
    public void setResponds(String oid, boolean responds)
    {
        createOrGetParam(oid).setResponse(responds);
    }

}
