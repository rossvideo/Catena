package com.rossvideo.catena.oauth;

public class SimpleOAuthConfig implements OAuthConfig
{
    private boolean validationRequired;
    private String clientID;
    private String publicKey;
    
    public SimpleOAuthConfig()
    {
        
    }
    
    public SimpleOAuthConfig(String publicKey, String clientID, boolean validationRequired)
    {
        this.publicKey = publicKey;
        this.clientID = clientID;
        this.validationRequired = validationRequired;
    }
    
    @Override
    public boolean isValidationRequired()
    {
        return validationRequired;
    }
    
    public void setValidationRequired(boolean validationRequired)
    {
        this.validationRequired = validationRequired;
    }
    
    public void setClientID(String clientID)
    {
        this.clientID = clientID;
    }
    
    @Override
    public String getClientID()
    {
        return clientID;
    }
    
    public void setPublicKey(String publicKey)
    {
        this.publicKey = publicKey;
    }

    @Override
    public String getPublicKey()
    {
        return publicKey;
    }

}
