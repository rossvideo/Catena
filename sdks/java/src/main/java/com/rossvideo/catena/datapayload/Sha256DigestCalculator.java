package com.rossvideo.catena.datapayload;

public class Sha256DigestCalculator extends DigestCalculator
{
    private static Sha256DigestCalculator instance = null;

    public static Sha256DigestCalculator getInstance()
    {
        if (instance == null)
        {
            instance = new Sha256DigestCalculator();
        }
        return instance;
    }
    
    private Sha256DigestCalculator()
    {
        super("SHA-256");
        // Prevent instantiation
    }
    

}
