package com.rossvideo.catena.utils;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class IOUtils
{

    public static byte[] readAllBytes(InputStream data) throws IOException
    {
        ByteArrayOutputStream streamBytes = new ByteArrayOutputStream();
        BufferedInputStream fis = null;
        try
        {
            fis = new BufferedInputStream(data);
            byte[] inBytes = new byte[1024];
            while (true)
            {
                int bytesRead = fis.read(inBytes);
                if (bytesRead < 0)
                {
                    break;
                }
                else if (bytesRead > 0)
                {
                    streamBytes.write(inBytes, 0, bytesRead);
                }
            }
        }
        finally
        {
            if (fis != null)
            {
                fis.close();
            }
        }
        
        return streamBytes.toByteArray();
    }

}
