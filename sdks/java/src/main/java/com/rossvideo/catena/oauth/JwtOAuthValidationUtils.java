package com.rossvideo.catena.oauth;

import java.security.KeyFactory;
import java.security.NoSuchAlgorithmException;
import java.security.PublicKey;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.X509EncodedKeySpec;
import java.util.Base64;
import java.util.Map;

import io.jsonwebtoken.Claims;
import io.jsonwebtoken.Jws;
import io.jsonwebtoken.JwtParser;
import io.jsonwebtoken.Jwts;

public class JwtOAuthValidationUtils implements OAuthValidationUtils
{
    private OAuthConfig config;
    private JwtParser parser;
    
    public JwtOAuthValidationUtils(OAuthConfig config)
    {
        this.config = config;
    }
    
    public JwtParser getParser() 
    {
        if (parser != null)
        {
            return parser;
        }
        
        String publicKeyStr = config.getPublicKey();
        if (publicKeyStr == null)
        {
            return null;
        }
        
        X509EncodedKeySpec keySpec = new X509EncodedKeySpec(Base64.getDecoder().decode(publicKeyStr));
        KeyFactory kf;
        try
        {
            kf = KeyFactory.getInstance("RSA");
            PublicKey publicKey = kf.generatePublic(keySpec);
            parser = Jwts.parser().verifyWith(publicKey).build();
            return parser;
        }
        catch (NoSuchAlgorithmException | InvalidKeySpecException e)
        {
            e.printStackTrace();
        }
        return null;
    }
    
    @Override
    public boolean isValidationRequired()
    {
        return config.isValidationRequired();
    }

    @Override
    public Map<String, Object> getAuthClaims(String authorizationHeaderValue)
    {
        JwtParser parser = getParser();
        if (parser == null) {
            return null;
        }
        
        if (authorizationHeaderValue != null && authorizationHeaderValue.startsWith("Bearer ")) {
            authorizationHeaderValue = authorizationHeaderValue.substring("Bearer ".length());
            try {
                Jws<Claims> claims = parser.parseSignedClaims(authorizationHeaderValue);
                return claims.getPayload();
            } catch (Exception e) {
                return null;
            }
        }
        
        return null;
    }

}
