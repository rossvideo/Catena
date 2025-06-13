package com.rossvideo.catena.datapayload;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;

import com.google.protobuf.ByteString;

import catena.core.parameter.DataPayload;

public abstract class AbstractFilePayloadReceiver
{
    private File currentFile;
    private FileOutputStream fileOutputStream;
    private int fileCount;
    private int skipCount;
    
    public abstract File getFileFor(String fileName);
    
    public void close()
    {
        closeRx();
    }
    
    private void closeRx()
    {
        if (fileOutputStream != null)
        {
            try
            {
                fileOutputStream.close();
            }
            catch (IOException e)
            {
                // Ignore
            }
        }
    }
    
    public int getFileCount()
    {
        return fileCount;
    }
    
    public int getSkipCount()
    {
        return skipCount;
    }
    
    public File getCurrentFile()
    {
        return currentFile;
    }
    
    public boolean isNewFile(DataPayload dataPayload)
    {
        ByteString digest = dataPayload.getDigest();
        String fileName = dataPayload.getMetadataOrDefault(DataPayloadBuilder.METADATA_FILE_NAME, null);
        return fileName != null || (digest != null && digest.size() > 0) || currentFile == null;
    }
    
    public void init(DataPayload dataPayload) throws IOException
    {
        closeRx();
        
        // TODO: use digest to verify the file integrity
        ByteString digest = dataPayload.getDigest();
        File file = getFileFor(dataPayload.getMetadataOrDefault(DataPayloadBuilder.METADATA_FILE_NAME, null));
        if (file == null)
        {
            throw new IOException("Unable to determine file to write to");
        }
        
        File parent = file.getParentFile();
        if (parent != null && !parent.exists()) {
            parent.mkdirs();
        }
        
        try
        {
            if (dataPayload.getPayloadEncoding() == DataPayload.PayloadEncoding.UNCOMPRESSED)
            {
                fileOutputStream = new FileOutputStream(file);
                currentFile = file;
                fileCount++;
            }
            else
            {
                throw new UnsupportedEncodingException("Unsupported payload encoding: " + dataPayload.getPayloadEncoding() +". Currently only uncompressed is supported.");
            }
        }
        catch (FileNotFoundException | UnsupportedEncodingException e)
        {
            throw new IOException("Unable to open " + file.getName() + " for writing", e);
        }
        
        writePayload(dataPayload);
    }
    
    public void writePayload(DataPayload dataPayload) throws IOException
    {
        if (fileOutputStream != null && dataPayload.getPayload() != null)
        {
            try
            {
                System.out.println("Writing to file: " + currentFile.getName() + ", size: " + dataPayload.getPayload().size());
                fileOutputStream.write(dataPayload.getPayload().toByteArray());
            }
            catch (IOException e)
            {
                throw new IOException("Unable to write to " + currentFile.getName(), e);
            }
        }
    }
}
