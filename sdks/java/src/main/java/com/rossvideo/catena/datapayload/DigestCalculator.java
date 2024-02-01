package com.rossvideo.catena.datapayload;

import java.io.IOException;
import java.io.InputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class DigestCalculator
{
    private static final int BUFFER_SIZE = 8192;
    private String algorithm;
    
    public DigestCalculator(String algorithm)
    {
        this.algorithm = algorithm;
    }
    
    public byte[] calculate(InputStream inputStream) throws IOException
    {
        try
        {
            MessageDigest digest = MessageDigest.getInstance(algorithm);
            byte[] buffer = new byte[BUFFER_SIZE];
            int amountRead;
            while ((amountRead = inputStream.read(buffer)) != -1)
            {
                digest.update(buffer, 0, amountRead);
            }
            return digest.digest();
        }
        catch (NoSuchAlgorithmException e)
        {
            throw new IOException("Unable to calculate digest", e);
        }
    }
    
}
