package com.rossvideo.catena.example.error;

import io.grpc.Status;
import io.grpc.StatusRuntimeException;

public class UnknownOidException extends StatusRuntimeException {

    private static final long serialVersionUID = 1L;

    public UnknownOidException(String oid) {
        super(Status.INVALID_ARGUMENT.withDescription("Unknown oid provided: " + oid));
    }
}
