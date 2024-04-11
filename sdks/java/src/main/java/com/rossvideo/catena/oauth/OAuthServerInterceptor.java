package com.rossvideo.catena.oauth;

import java.util.Map;

import com.rossvideo.catena.device.CatenaServer;

import io.grpc.Context;
import io.grpc.Contexts;
import io.grpc.Metadata;
import io.grpc.ServerCall;
import io.grpc.ServerCallHandler;
import io.grpc.ServerInterceptor;
import io.grpc.Status;

public class OAuthServerInterceptor implements ServerInterceptor
{
    private OAuthValidationUtils utils;
    
    public OAuthServerInterceptor(OAuthValidationUtils utils)
    {
        this.utils = utils;
    }

    @Override
    public <ReqT, RespT> ServerCall.Listener<ReqT> interceptCall(ServerCall<ReqT, RespT> call, Metadata headers, ServerCallHandler<ReqT, RespT> next) {
        String authorizationHeaderValue = headers.get(Metadata.Key.of("authorization", Metadata.ASCII_STRING_MARSHALLER));
        Map<String, Object> authClaims = utils.getAuthClaims(authorizationHeaderValue);
        if (authClaims != null) {
            // Token is valid, proceed with the call
            Context context = Context.current().withValue(CatenaServer.KEY_CLAIMS, authClaims);
            return Contexts.interceptCall(context, call, headers, next);
        } else if (utils.isValidationRequired()) {
            // Token is invalid, close the call with UNAUTHENTICATED status
            call.close(Status.PERMISSION_DENIED.withDescription("Invalid or missing token"), headers);
            return new ServerCall.Listener<ReqT>() {};
        }
        
        return next.startCall(call, headers);
    }

}
