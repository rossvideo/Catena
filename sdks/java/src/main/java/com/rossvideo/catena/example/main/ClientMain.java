package com.rossvideo.catena.example.main;

import java.io.File;

import com.rossvideo.catena.example.client.MyCatenaClient;

public class ClientMain {

    public static final int DEFAULT_SLOT = 1;
    public static final int DEFAULT_PORT = 6254;

    public static void main(String[] args) {
        File clientWorkingDirectory = null;
        String hostname = "127.0.0.1";
        int port = 6254;
        boolean secure = false;
        
        if (args.length > 0) {
            clientWorkingDirectory = new File(args[0]);
            clientWorkingDirectory.mkdirs();
        }
        
        if (args.length > 1) {
            hostname = args[1];
        }
        
        if (args.length > 2) {
            port = Integer.parseInt(args[2]);  
        }
                
        if (args.length > 3) {
            secure = "-secure".equals(args[3]);
        }
        
        try (MyCatenaClient client = new MyCatenaClient(hostname, port, clientWorkingDirectory, secure)) {
            client.start();

            CommandExecutor.executeOnClient(client, DEFAULT_SLOT);

        } catch (Exception e) {
            System.err.println("Exception occurred." + e.getLocalizedMessage());
            System.err.flush();
            e.printStackTrace();
        }
    }
}
