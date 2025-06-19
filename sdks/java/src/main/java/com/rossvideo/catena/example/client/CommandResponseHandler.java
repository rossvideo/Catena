package com.rossvideo.catena.example.client;

import java.io.File;
import java.io.IOException;
import java.util.Iterator;

import com.rossvideo.catena.datapayload.FilePayloadReceiver;

import catena.core.parameter.CommandResponse;
import catena.core.parameter.DataPayload;
import catena.core.parameter.Value;

public final class CommandResponseHandler {

	private File workingDirectory;

    public CommandResponseHandler(File workingDirectory) {
		this.workingDirectory = workingDirectory;
	}

    public void printCommandResponse(CommandResponse commandResponse) {
		switch (commandResponse.getKindCase()) {
			case KIND_NOT_SET:
				System.out.println("CLIENT: Server did not set the type of the CommandResponse.");
				break;	
			case EXCEPTION:
				System.out.println("CLIENT: Server sent back an exception: " + commandResponse.getException());
				break;
			case NO_RESPONSE:
				System.out.println("CLIENT: Server sent no response.");
				break;
			case RESPONSE:
				System.out.println("CLIENT: Server sent response: " + commandResponse.getResponse());
				break;
			default:
				break;
		}
	}

    public void receiveFile(Iterator<CommandResponse> commandResponses) {
		if (commandResponses == null || !commandResponses.hasNext()) {
			System.out.println("CLIENT: No command responses to process.");
			return;
		}
		FilePayloadReceiver filePayloadReceiver = new FilePayloadReceiver(workingDirectory, null);

		try {
			
			CommandResponse commandResponse = commandResponses.next();
			Value response = commandResponse.getResponse();
			if(response==null || !response.hasDataPayload()) {
				System.out.println("CLIENT: Server did not respond with file data payload.");
				return;
			}
			DataPayload payload = response.getDataPayload();
			filePayloadReceiver.init(payload);

			while (commandResponses.hasNext()) {
				commandResponse = commandResponses.next();
				response = commandResponse.getResponse();
				if (response == null || !response.hasDataPayload()) {
					System.out.println("CLIENT: Server did not respond with file data payload.");
					continue;
				}
				payload = response.getDataPayload();
				filePayloadReceiver.writePayload(payload);
			}

			String fileName = filePayloadReceiver.getCurrentFile().getName();
			System.out.println("CLIENT: Received file: " + fileName);
			filePayloadReceiver.close();

			} catch (IOException e) {
				System.err.println("CLIENT: Error processing file payload: " + e.getMessage());
			}
    }
}
