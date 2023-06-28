package com.rossvideo.catena.example.error;

import io.grpc.Status;
import io.grpc.StatusRuntimeException;

public class InvalidSlotNumberException extends StatusRuntimeException {
	
	private static final long serialVersionUID = 1L;

	public InvalidSlotNumberException(int slot) {
		super(Status.INVALID_ARGUMENT.withDescription("Invalid slot number provided: " + slot));
	}
}
