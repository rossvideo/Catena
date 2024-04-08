package com.rossvideo.catena.device.impl;

import catena.core.language.PolyglotText;

public class TextUtils
{
    public static String defaultLanguage = "en";
    
    public static PolyglotText createSimpleText(String text) {
        return createSimpleText(defaultLanguage, text);
    }
    
    public static PolyglotText createSimpleText(String language, String text) {
        return PolyglotText.newBuilder().putDisplayStrings(language, text).build();
    }
    
    public static PolyglotText.Builder putDisplayStrings(PolyglotText start, String language, String text)
    {
        PolyglotText.Builder builder = start.toBuilder();

        builder.putDisplayStrings(language, text);

        return builder;
    }
    
    public static PolyglotText.Builder putDisplayStrings(String language, String text)
    {
        PolyglotText.Builder builder = PolyglotText.newBuilder();
        
        builder.putDisplayStrings(language, text);
        
        return builder;
    }
}
