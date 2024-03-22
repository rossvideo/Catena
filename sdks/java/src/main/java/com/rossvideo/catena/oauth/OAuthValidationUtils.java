package com.rossvideo.catena.oauth;

import java.util.Map;

public interface OAuthValidationUtils
{
    public boolean isValidationRequired();
    public Map<String, Object> getAuthClaims(String authorizationHeaderValue);

}
