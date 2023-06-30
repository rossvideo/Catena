package com.rossvideo.catena.example.main;

import com.rossvideo.catena.example.MyCatenaDevice;

import io.grpc.Server;
import io.grpc.ServerBuilder;

public class ServerMain {
	
	private static int port = 5255;
	private static int slotNumber = 1;
	
	public static void main(String[] args) {
		try {
			Server server = ServerBuilder.forPort(port).addService(new MyCatenaDevice(slotNumber)).build();
			server.start();
			System.out.println("SERVER: Server started.");
			server.awaitTermination();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
