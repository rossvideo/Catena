package com.rossvideo.catena.example.client.command;

import catena.core.parameter.CommandResponse;
import io.grpc.stub.StreamObserver;

/**
 * Handles receiving CommandResponse instances
 */
// public class CommandResponseHandler implements StreamObserver<CommandResponse> {

// 	@Override
// 	public void onNext(CommandResponse commandResponse) {
// 		switch (commandResponse.getKindCase()) {
// 			case KIND_NOT_SET:
// 				System.out.println("CLIENT: Server did not set the type of the CommandResponse.");
// 				break;	
// 			case EXCEPTION:
// 				System.out.println("CLIENT: Server sent back an exception: " + commandResponse.getException());
// 				break;
// 			case NO_RESPONSE:
// 				System.out.println("CLIENT: Server sent no response.");
// 				break;
// 			case RESPONSE:
// 				System.out.println("CLIENT: Server sent response: " + commandResponse.getResponse());
// 				break;
// 			default:
// 				break;
// 		}
// 	}

// 	@Override
// 	public void onError(Throwable t) {
// 		System.out.println("CLIENT: Server returned an error: " + t.getLocalizedMessage());
// 		t.printStackTrace();
// 	}

// 	@Override
// 	public void onCompleted() {
// 		System.out.println("CLIENT: Server finished sending command responses.");
// 	}

// }
