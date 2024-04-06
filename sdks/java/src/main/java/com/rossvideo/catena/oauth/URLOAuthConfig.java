package com.rossvideo.catena.oauth;

import java.io.IOException;
import java.lang.reflect.Type;
import java.net.URL;
import java.util.Map;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;
import com.rossvideo.catena.utils.URLUtils;

public class URLOAuthConfig extends SimpleOAuthConfig
{
    public void initFrom(URL serverConfigURL) throws IOException
    {
        String serverConfig = URLUtils.readURLFullyAsString(serverConfigURL);
        Gson gson = new Gson();
        Type mapType = new TypeToken<Map<String, Object>>(){}.getType();
        Map<String, Object> map = gson.fromJson(serverConfig, mapType);
        setPublicKey((String)map.get("public_key"));
    }
    
    public void initFrom(URL serverConfigURL, String clientID, boolean requireValidation) throws IOException
    {
        setClientID(clientID);
        setValidationRequired(requireValidation);
        initFrom(serverConfigURL);
    }
}
