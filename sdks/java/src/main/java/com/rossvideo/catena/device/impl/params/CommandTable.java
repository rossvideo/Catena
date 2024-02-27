package com.rossvideo.catena.device.impl.params;

import java.util.Map.Entry;
import java.util.Set;

import catena.core.device.Device.Builder;
import catena.core.parameter.Param;

public class CommandTable extends ParamTable
{

    @Override
    protected Set<Entry<String, Param>> getParams(Builder builder)
    {
        return builder.getCommandsMap().entrySet();
    }

    @Override
    protected void commitTopLevelParam(Builder deviceBuilder, String oid, Param param)
    {
        deviceBuilder.putCommands(oid,  param);
    }

    
}
