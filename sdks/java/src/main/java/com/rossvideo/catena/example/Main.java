package com.rossvideo.catena.example;

import io.grpc.Server;
import io.grpc.ServerBuilder;

public class Main {

	private static int port = 5255;
	private static int slotNumber = 1;
	private static boolean run = true;
	private static Object lock = new Object();

	public static void main(String[] args) {
		new Thread(() -> {
			try {
				Server server = ServerBuilder.forPort(port).addService(new MyCatenaDevice(slotNumber)).build();
				server.start();
				
				while (run) {
					synchronized(lock) {
						lock.wait();
					}
				}
				
				System.out.println("SERVER: Shutting down Server.");
				server.shutdownNow();
				server.awaitTermination();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}).start();

		new Thread(() -> {
			
			try (MyCatenaClient client = new MyCatenaClient("localhost", port)) {
				client.start();
				
				// Tests for int parameter
				System.out.println("TEST: Read initial int value.");
				client.getValue(MyCatenaDevice.INT_OID, slotNumber);
				
				int intValue = 42;
				System.out.println("TEST: Set int value to " + intValue);
				client.setValue(MyCatenaDevice.INT_OID, slotNumber, intValue);
				
				System.out.println("TEST: Read int value.");
				client.getValue(MyCatenaDevice.INT_OID, slotNumber);
				
				// Tests for float parameter
				System.out.println("TEST: Read initial float value.");
				client.getValue(MyCatenaDevice.FLOAT_OID, slotNumber);
				
				float floatValue = 42.42F;
				System.out.println("TEST: Set float value to " + floatValue);
				client.setValue(MyCatenaDevice.FLOAT_OID, slotNumber, floatValue);
				
				System.out.println("TEST: Read float value.");
				client.getValue(MyCatenaDevice.FLOAT_OID, slotNumber);
				
				// Tests for set/get parameter error cases
				String wrongOid = "blah_blah_blah";
				System.out.println("TEST: Read value with wrong oid: " + wrongOid);
				client.getValue(wrongOid, slotNumber);
				
				int wrongSlotNumber = 12345;
				System.out.println("TEST: Read value with wrong slot number: " + wrongSlotNumber);
				client.getValue(MyCatenaDevice.INT_OID, wrongSlotNumber);
				
				System.out.println("TEST: Setting int value on a float type OID.");
				client.setValue(MyCatenaDevice.FLOAT_OID, slotNumber, 42);
				
				// Tests for getDevice
				System.out.println("TEST: GetDevice for slot: " + slotNumber);
				client.getDevice(slotNumber);
				
				System.out.println("TEST: GetDevice for wrong slot: " + wrongSlotNumber);
				client.getDevice(wrongSlotNumber);
				
				System.out.println("TEST: Complete.");

			} catch (Exception e) {
				System.err.println("Exception occurred." + e.getLocalizedMessage());
				System.err.flush();
				e.printStackTrace();
			} finally {
				synchronized(lock) {
					run = false;
					lock.notifyAll();
				}
			}
		}).start();
	}

}
