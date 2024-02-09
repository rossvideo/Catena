package com.rossvideo.catena.example.main;

import java.io.File;

import com.rossvideo.catena.example.client.MyCatenaClient;

public class ClientMain {

    private static int port = 5255;
    private static int slotNumber = 1;

    public static void main(String[] args) {
        File clientWorkingDirectory = null;
        if (args.length > 0) {
            clientWorkingDirectory = new File(args[0]);
            clientWorkingDirectory.mkdirs();
        }
        
        try (MyCatenaClient client = new MyCatenaClient("127.0.0.1", port, clientWorkingDirectory)) {
            client.start();

            CommandExecutor.executeOnClient(client, slotNumber);

        } catch (Exception e) {
            System.err.println("Exception occurred." + e.getLocalizedMessage());
            System.err.flush();
            e.printStackTrace();
        }
    }
}
