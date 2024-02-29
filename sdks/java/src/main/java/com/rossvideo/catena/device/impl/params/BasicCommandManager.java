package com.rossvideo.catena.device.impl.params;

import com.rossvideo.catena.device.impl.CommandManager;

import catena.core.device.Device;
import catena.core.device.Device.Builder;
import catena.core.parameter.Param;

public class BasicCommandManager extends BasicParamManager implements CommandManager
{

    public BasicCommandManager(Builder deviceBuilder)
    {
        super(deviceBuilder);
    }
    
    protected void putTopLevelParam(String fieldName, Param param)
    {
        getDeviceBuilder().putCommands(fieldName, param);
    }
    
    protected Param.Builder getBuilderForTopLevelParam(String fieldName)
    {
        Device.Builder deviceBuilder = getDeviceBuilder();
        Param existing = deviceBuilder.getCommandsOrDefault(fieldName, null);
        deviceBuilder.removeParams(fieldName);
        Param.Builder structParamBuilder = deviceBuilder.putParamsBuilderIfAbsent(fieldName);
        if (existing != null) {
            structParamBuilder.mergeFrom(existing);                        
        }
        
        return structParamBuilder;
    }
}
