package com.rossvideo.catena.example.main;

import java.util.HashMap;
import java.util.Map;

public class MainUtil
{
    public static boolean booleanFromMap(Map<String, String> map, String key, boolean defaultValue)
    {
        String value = map.get(key);
        if (value != null)
        {
            return Boolean.parseBoolean(value);
        }
        return defaultValue;
    }
    
    public static String stringFromMap(Map<String, String> map, String key, String defaultValue)
    {
        String value = map.get(key);
        if (value != null)
        {
            return value;
        }
        return defaultValue;
    }
    
    public static int intFromMap(Map<String, String> map, String key, int defaultValue)
    {
        String value = map.get(key);
        if (value != null)
        {
            try
            {
                return Integer.parseInt(value);
            }
            catch (NumberFormatException e)
            {
                return defaultValue;
            }
        }
        return defaultValue;
    }
    
    public static Map<String, String> parseArguments(String[] args) {
        Map<String, String> arguments = new HashMap<>();
        
        for (int i = 0; i < args.length; i++) {
            String arg = args[i];
            if (arg.startsWith("--")) {
                // For --key=value
                String[] parts = arg.substring(2).split("=", 2);
                if (parts.length == 2) {
                    arguments.put(parts[0], parts[1]);
                }
            } else if (arg.startsWith("-")) {
                // For -key value
                String key = arg.substring(1);
                if (i + 1 < args.length) {
                    String value = args[i + 1];
                    arguments.put(key, value);
                    i++; // Skip next because it's already used as a value
                }
            }
        }

        return arguments;
    }
}
