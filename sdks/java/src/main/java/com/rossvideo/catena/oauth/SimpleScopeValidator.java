package com.rossvideo.catena.oauth;

public class SimpleScopeValidator implements ScopeValidator
{
    private String[] userScopes;
    
    public static SimpleScopeValidator getFrom(String scopes)
    {
        if (scopes == null || scopes.isEmpty())
        {
            return new SimpleScopeValidator(new String[0]);
        }
        else
        {
            return new SimpleScopeValidator(scopes);
        }
    }
    
    
    public SimpleScopeValidator(String userScopes)
    {
        this(userScopes == null ? new String[0] : userScopes.trim().split(" "));
    }
    
    public SimpleScopeValidator(String[] userScopes)
    {
        this.userScopes = userScopes;
    }
    
    @Override
    public boolean isInScope(String scope)
    {
        for (String userScope : userScopes)
        {
            if (userScope.equals(scope))
            {
                return true;
            }
        }
        return false;
    }
}
