package com.rossvideo.catena.example.error;

import catena.core.Value.KindCase;
import io.grpc.Status;
import io.grpc.StatusRuntimeException;

public class WrongValueTypeException extends StatusRuntimeException {
	
	private static final long serialVersionUID = 1L;

	private static String errorMessageFormat = "Wrong value type provided.  Got: %s, expected: %s";

	public WrongValueTypeException(KindCase valueType, KindCase expected) {
		super(Status.INVALID_ARGUMENT.withDescription(String.format(errorMessageFormat, valueType.name(), expected.name())));
	}
}
