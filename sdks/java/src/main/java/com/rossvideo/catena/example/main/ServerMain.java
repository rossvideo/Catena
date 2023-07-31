package com.rossvideo.catena.example.main;

import com.rossvideo.catena.example.MyCatenaDevice;

import io.grpc.Grpc;
import io.grpc.InsecureServerCredentials;
import io.grpc.Server;
import io.grpc.ServerCredentials;

public class ServerMain {

	private static int port = 5255;
	private static int slotNumber = 1;

	public static void main(String[] args) {
		try {
			ServerCredentials credentials = InsecureServerCredentials.create(); // TODO: Create proper credentials.
			Server server = Grpc.newServerBuilderForPort(port, credentials)
					.addService(new MyCatenaDevice(slotNumber))
					.build();
			server.start();
			System.out.println("SERVER: Server started.");
			server.awaitTermination();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
