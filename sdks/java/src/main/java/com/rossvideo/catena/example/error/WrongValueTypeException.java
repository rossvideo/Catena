package com.rossvideo.catena.example.error;

import catena.core.parameter.Value.KindCase;
import io.grpc.Status;
import io.grpc.StatusRuntimeException;

public class WrongValueTypeException extends StatusRuntimeException {

    private static final long serialVersionUID = 1L;

    private static String errorMessageFormat =
      "Wrong value type provided for OID: %s.  Got: %s, expected: %s";

    public WrongValueTypeException(String oid, KindCase valueType, KindCase expectedType) {
        super(Status.INVALID_ARGUMENT.withDescription(
          String.format(errorMessageFormat, oid, valueType.name(), expectedType.name())));
    }
}
