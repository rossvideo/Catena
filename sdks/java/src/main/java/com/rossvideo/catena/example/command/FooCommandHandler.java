package com.rossvideo.catena.example.command;

import com.rossvideo.catena.example.MyCatenaDevice;
import com.rossvideo.catena.example.error.InvalidSlotNumberException;
import com.rossvideo.catena.example.error.UnknownOidException;
import com.rossvideo.catena.example.error.WrongValueTypeException;

import catena.core.parameter.CommandResponse;
import catena.core.parameter.ExecuteCommandPayload;
import catena.core.parameter.Value;
import catena.core.parameter.Value.KindCase;
import io.grpc.StatusRuntimeException;
import io.grpc.stub.StreamObserver;

/**
 * Handles receiving ExecuteCommandPayload instances on the server side to
 * provide responses back to the client.
 */
public class FooCommandHandler implements StreamObserver<ExecuteCommandPayload> {

	private final int slot;
	private final StreamObserver<CommandResponse> responseObserver;

	public FooCommandHandler(int slot, StreamObserver<CommandResponse> responseObserver) {
		this.slot = slot;
		this.responseObserver = responseObserver;
	}

	@Override
	public void onNext(ExecuteCommandPayload commandPayload) {
		try {
			CommandResponse commandResponse = processCommandPayload(commandPayload);
			if (commandResponse != null) {
				responseObserver.onNext(commandResponse);
			}
	        responseObserver.onCompleted();
		} catch (StatusRuntimeException e) {
			responseObserver.onError(e);
		}
	}

	@Override
	public void onError(Throwable t) {
		System.out.println("SERVER: Received an error from client: " + t.getLocalizedMessage());
		t.printStackTrace(System.out);
	}

	@Override
	public void onCompleted() {
		System.out.println("SERVER: Client finished sending command stream.");
	}

	/**
	 * Processes an ExecuteCommandPayload to return either a CommandResponse
	 * instance or null.<br>
	 * Throws an exception if there is a problem with one of the values.
	 * 
	 * @param commandPayload The command payload to process.
	 * @return A CommandResponse if respond is set to true in commandPayload or null
	 *         if it's false.
	 */
	private CommandResponse processCommandPayload(ExecuteCommandPayload commandPayload) {

		int commandSlot = commandPayload.getSlot();
		String oid = commandPayload.getOid();
		Value value = commandPayload.getValue();
		boolean respond = commandPayload.getRespond();

		System.out.println("SERVER: processing command:");
		System.out.println("\tslot:    " + commandSlot);
		System.out.println("\toid:     " + oid);
		System.out.println("\trespond: " + respond);
		System.out.println("\tvalue:   " + value);

		if (this.slot != commandSlot) {
			throw new InvalidSlotNumberException(commandSlot);
		}

		KindCase valueKindCase = value.getKindCase();
			switch (oid) {
			case MyCatenaDevice.CMD_FOO_OID:
				if (valueKindCase != KindCase.STRING_VALUE) {
					throw new WrongValueTypeException(oid, valueKindCase, KindCase.STRING_VALUE);
				}
				break;
			default:
				throw new UnknownOidException(oid);
		}

		// TODO handle the case where an internal server error prevents sending a response
		// return CommandResponse.newBuilder().setException(
		// Exception.newBuilder().setType("TheType").setDetails("Details").setErrorMessage(
		// PolyglotText.newBuilder().setMonoglot("Monoglot").build()
		// ).build()).build();
		
		// TODO handle the case if the Server/Device has no response to send back.
		// return CommandResponse.newBuilder().setNoResponse(Empty.getDefaultInstance()).build();
		
		if (respond) {
			return CommandResponse.newBuilder()
					.setResponse(Value.newBuilder().setStringValue("YOU SENT \"" + value.getStringValue() + "\"").build())
					.build();
		} else {
			return null;
		}
	}
}
