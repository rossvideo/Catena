package com.rossvideo.catena.example.main;

import com.rossvideo.catena.example.MyCatenaClient;

public class ClientMain {

    private static int port = 5255;
    private static int slotNumber = 1;

    public static void main(String[] args) {
        try (MyCatenaClient client = new MyCatenaClient("127.0.0.1", port)) {
            client.start();

            CommandExecutor.executeOnClient(client, slotNumber);

        } catch (Exception e) {
            System.err.println("Exception occurred." + e.getLocalizedMessage());
            System.err.flush();
            e.printStackTrace();
        }
    }
}
