package com.rossvideo.catena.oauth;

public interface ScopeValidator
{

    public boolean isInScope(String scope);

    public default boolean isReadable(String paramScope) { return isInScope(paramScope) || isInScope(paramScope + ":w"); };

    public default boolean isWritable(String paramScope) { return isInScope(paramScope + ":w"); };

}