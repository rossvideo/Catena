package com.rossvideo.catena.utils;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

public class URLUtils
{
    public static String readURLFullyAsString(URL url) throws IOException
    {
        return new String(readURLFullyAsBytes(url, false), "UTF-8");
    }
    
    public static byte[] readURLFullyAsBytes(URL tokenURL, boolean asPost) throws IOException
    {
        HttpURLConnection connection = (HttpURLConnection)tokenURL.openConnection();
        if (asPost && tokenURL.getQuery() != null)
        {
            connection.setRequestMethod("POST");
            connection.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");
            connection.setDoOutput(true);
            connection.getOutputStream().write(tokenURL.getQuery().getBytes("UTF-8"));
            connection.getOutputStream().flush();
            connection.getOutputStream().close();
        }
        InputStream httpInput = connection.getInputStream();
        return IOUtils.readAllBytes(httpInput);
    }
}
