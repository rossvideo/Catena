package com.rossvideo.catena.example.error;

import io.grpc.Status;
import io.grpc.StatusRuntimeException;

public class UnimplementedFeatureException extends StatusRuntimeException
{
    /**
     * 
     */
    private static final long serialVersionUID = 867657673467640712L;

    public UnimplementedFeatureException(String message)
    {
        super(Status.UNIMPLEMENTED.withDescription(message));
    }

}
