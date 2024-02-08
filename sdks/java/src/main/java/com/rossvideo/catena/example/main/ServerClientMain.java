package com.rossvideo.catena.example.main;

import java.io.File;

import com.rossvideo.catena.example.client.MyCatenaClient;
import com.rossvideo.catena.example.device.MyCatenaDevice;

import io.grpc.Grpc;
import io.grpc.InsecureServerCredentials;
import io.grpc.Server;
import io.grpc.ServerCredentials;

public class ServerClientMain {

    private static int port = 5255;
    private static int slotNumber = 1;
    private static boolean run = true;
    private static Object lock = new Object();

    public static void main(String[] args) throws Exception {
        File clientWorkingDirectory = null;
        File serverWorkingDirectory = null;
        if (args.length > 1) {
            clientWorkingDirectory = new File(args[0]);
            clientWorkingDirectory.mkdirs();

            serverWorkingDirectory = new File(args[1]);
            serverWorkingDirectory.mkdirs();
        }
        
        final File fClientWorkingDirectory = clientWorkingDirectory;
        final File fServerWorkingDirectory = serverWorkingDirectory;
        
        new Thread(() -> {
            try {
                ServerCredentials credentials =
                  InsecureServerCredentials.create();  // TODO: Create proper credentials.
                Server server = Grpc.newServerBuilderForPort(port, credentials)
                                  .addService(new MyCatenaDevice(slotNumber, fClientWorkingDirectory))
                                  .build();
                server.start();

                synchronized (lock) {
                    while (run) {
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

        Thread.sleep(2000);  // Let some time for the server to start first

        new Thread(() -> {
            try (MyCatenaClient client = new MyCatenaClient("localhost", port, fServerWorkingDirectory)) {
                client.start();
                CommandExecutor.executeOnClient(client, slotNumber);

            } catch (Exception e) {
                System.err.println("Exception occurred." + e.getLocalizedMessage());
                System.err.flush();
                e.printStackTrace();
            } finally {
                synchronized (lock) {
                    run = false;
                    lock.notifyAll();
                }
            }
        }).start();
    }
}
