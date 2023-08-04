package com.rossvideo.catena.example.main;

import com.rossvideo.catena.example.MyCatenaClient;
import com.rossvideo.catena.example.MyCatenaDevice;

public class ClientMain {

    private static int port = 5255;
    private static int slotNumber = 1;

    public static void main(String[] args) {
        try (MyCatenaClient client = new MyCatenaClient("127.0.0.1", port)) {
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
        }
    }
}
