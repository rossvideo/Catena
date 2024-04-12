package com.rossvideo.catena.device;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.util.JsonFormat;
import com.rossvideo.catena.utils.IOUtils;

import catena.core.device.Device;
import catena.core.device.Device.Builder;

public class JsonCatenaDevice extends BasicCatenaDevice
{
    private Device.Builder deviceBuilder;
    
    public JsonCatenaDevice(CatenaServer server)
    {
        super(server);
    }
    
    public void init(File jsonDefinition) throws IOException {
        try (FileInputStream fis = new FileInputStream(jsonDefinition))
        {
            init(fis);
        }
    }
    
    public void init(InputStream jsonDefinition) throws IOException {
        init(new String(IOUtils.readAllBytes(jsonDefinition), "UTF-8"));
    }
    
    public void init(String jsonDefinition) throws InvalidProtocolBufferException {
        if (deviceBuilder != null) {
            throw new IllegalStateException("Device already initialized");
        }
        deviceBuilder = Device.newBuilder();
        JsonFormat.parser().merge(jsonDefinition, deviceBuilder);
        setSlot(deviceBuilder.getSlot());
        super.init();
    }

    @Override
    protected Builder createDeviceBuilder()
    {
        return deviceBuilder;
    }
    
}
