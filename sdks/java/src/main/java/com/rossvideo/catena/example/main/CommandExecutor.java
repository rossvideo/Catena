package com.rossvideo.catena.example.main;

import java.io.IOException;

import com.rossvideo.catena.example.client.MyCatenaClient;
import com.rossvideo.catena.example.device.MyCatenaDevice;

/**
 * Deliberately package-private
 */
final class CommandExecutor {
	
	private CommandExecutor() {}
	
    static void executeOnClient(MyCatenaClient client, int slotNumber) throws InterruptedException, IOException {
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

        // Tests for commands
        System.out.println("TEST: Execute command \"foo('bar')\" with string value");
        client.executeCommand("foo", slotNumber, "bar", true);
        Thread.sleep(250);
        
        System.out.println("TEST: Execute command \"foo('bar')\" with wrong slot number");
        client.executeCommand("foo", -1, "bar", true);
        Thread.sleep(250);
        
        System.out.println("TEST: Execute command \"foo('bar')\" with wrong OID");
        client.executeCommand(MyCatenaDevice.FLOAT_OID, slotNumber, "bar", true);
        Thread.sleep(250);
        
        System.out.println("TEST: Execute command \"foo('bar')\" with wrong value type");
        client.executeCommand("foo", slotNumber, 42.42f, true);
        Thread.sleep(250);
        
        System.out.println("TEST: Execute command \"foo('bar')\" with no response expected");
        client.executeCommand("foo", slotNumber, "bar", false);
        Thread.sleep(250);

        System.out.println("TEST: Execute command \"reverse('hello world')\" with no response expected");
        client.executeCommand("reverse", slotNumber, "hello world", false);
        Thread.sleep(250);
        
        System.out.println("TEST: Push file to server");
        client.pushFile("file-receive", slotNumber, CommandExecutor.class.getResource("/files/sample-1.jpg"));
        
        System.out.println("TEST: Push same file to server");
        client.pushFile("file-receive", slotNumber, CommandExecutor.class.getResource("/files/sample-1.jpg"));
        
        System.out.println("TEST: Recieve file from server");
        client.receiveFile("file-transmit", slotNumber, "sample-file.jpg");
        
        // Tests for getDevice
        System.out.println("TEST: GetDevice for slot: " + slotNumber);
        client.getDevice(slotNumber);

        System.out.println("TEST: GetDevice for wrong slot: " + wrongSlotNumber);
        client.getDevice(wrongSlotNumber);

        System.out.println("TEST: Complete.");
    }

}
