package com.rossvideo.catena.datapayload;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

public class ResetableFileInputStream extends InputStream
{
    private File file;
    private InputStream inputStream;
    public ResetableFileInputStream(File file)
    {
        this.file = file;
    }
    
    public void checkStream() throws IOException
    {
        if (inputStream == null)
        {
            reset();
        }
    }
    
    @Override
    public synchronized void reset() throws IOException
    {
        if (inputStream != null)
        {
            inputStream.close();
        }
        
        inputStream = new FileInputStream(file);
    }

    

    @Override
    public int read(byte[] b) throws IOException
    {
        checkStream();
        return inputStream.read(b);
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException
    {
        checkStream();
        return inputStream.read(b, off, len);
    }

    @Override
    public long skip(long n) throws IOException
    {
        checkStream();
        return inputStream.skip(n);
    }



    @Override
    public int available() throws IOException
    {
        checkStream();
        return inputStream.available();
    }



    @Override
    public void close() throws IOException
    {
        checkStream();
        inputStream.close();
    }



    @Override
    public synchronized void mark(int readlimit)
    {
        
    }



    @Override
    public boolean markSupported()
    {
        return true;
    }



    @Override
    public int read() throws IOException
    {
        checkStream();
        return inputStream.read();
    }
    
}
