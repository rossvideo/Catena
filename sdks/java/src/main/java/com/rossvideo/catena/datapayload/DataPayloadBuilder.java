package com.rossvideo.catena.datapayload;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;

import com.google.protobuf.ByteString;

import catena.core.parameter.DataPayload;
import catena.core.parameter.DataPayload.Builder;

public class DataPayloadBuilder
{
    public static final String METADATA_CONTENT_TYPE = "Content-Type";
    public static final String METADATA_FILE_NAME = "File-Name";
    private long position = -1;
    private InputStream payload;
    private String name;
    private String mimeType;
    private byte[] digest;
    boolean done = false;
    
    public DataPayloadBuilder(InputStream payload, String name, String mimeType)
    {
        this.payload = payload;
        this.name = name;
        this.mimeType = mimeType;
    }
    
    public DataPayloadBuilder(File file, String mimeType)
    {
        this(new ResetableFileInputStream(file), file.getName(), mimeType);
    }

    public DataPayload createCompleteFileDataPayload() throws IOException
    {        
        DataPayload.Builder message = DataPayload.newBuilder();
        message.putMetadata(METADATA_FILE_NAME, name);
        message.putMetadata(METADATA_CONTENT_TYPE, mimeType); 
        
        if (!hasDigest())
        {
            calculateDigest();
            message.setDigest(ByteString.copyFrom(digest));
        }

        byte[] data = readAllBytes(payload);
        message.setPayload(ByteString.copyFrom(data));
        
        return message.build();
    }

    private static byte[] readAllBytes(InputStream in) throws IOException {
    ByteArrayOutputStream buffer = new ByteArrayOutputStream();
    byte[] data = new byte[8192];
    int nRead;
    while ((nRead = in.read(data, 0, data.length)) != -1) {
        buffer.write(data, 0, nRead);
    }
    return buffer.toByteArray();
}
    
    public void setDigest(byte[] digest)
    {
        this.digest = digest;
    }
    
    public void calculateDigest() throws IOException
    {
        if (!payload.markSupported())
        {
            payload = new BufferedInputStream(payload);
        }
        payload.mark(Integer.MAX_VALUE);
        setDigest(Sha256DigestCalculator.getInstance().calculate(payload));
        payload.reset();
    }
    
    public void close()
    {
        try
        {
            payload.close();
        }
        catch (IOException e)
        {
            // Ignore
        }
    }
    
    public DataPayload buildNext() throws IOException
    {
        DataPayload.Builder message = DataPayload.newBuilder();
        if (position < 0)
        {
            position = 0;
            message.putMetadata(METADATA_FILE_NAME, name);
            if (mimeType != null)
            {
                message.putMetadata(METADATA_CONTENT_TYPE, mimeType);                
            }
            
            if (digest != null)
            {
                message.setDigest(ByteString.copyFrom(digest));
            }
            else
            {
                done = putNextChunk(message) < 0;
            }
        }
        else
        {
            done = putNextChunk(message) < 0;
        }
        
        if (!done)
        {
            return message.build();
        }
        
        return null;
    }
    
    private int putNextChunk(Builder message) throws IOException
    {
        byte[] buffer = new byte[1024 * 1024];
        int readAmount = payload.read(buffer);
        if (readAmount > 0)
        {
            message.putMetadata("position", Long.toString(position));
            message.setPayload(ByteString.copyFrom(buffer, 0, readAmount));
            position += readAmount;
        }
        
        return readAmount;
    }
    
    public boolean hasDigest()
    {
        return digest != null;
    }

    public boolean hasNext()
    {
        try
        {
            return !done && payload.available() > 0;
        }
        catch (IOException e)
        {
            return false;
        }
    }
}
