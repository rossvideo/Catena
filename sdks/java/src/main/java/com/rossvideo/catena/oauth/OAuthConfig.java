package com.rossvideo.catena.oauth;

public interface OAuthConfig
{
    public boolean isValidationRequired();
    public String getClientID();
    public String getPublicKey();
}
