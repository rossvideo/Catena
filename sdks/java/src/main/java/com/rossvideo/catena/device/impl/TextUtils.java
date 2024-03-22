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
}
