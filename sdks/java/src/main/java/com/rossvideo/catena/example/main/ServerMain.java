package com.rossvideo.catena.example.main;

import java.io.File;

import javax.net.ssl.SSLException;

import com.rossvideo.catena.device.CatenaServer;
import com.rossvideo.catena.example.device.MyCatenaDevice;

import io.grpc.Grpc;
import io.grpc.InsecureServerCredentials;
import io.grpc.Server;
import io.grpc.ServerBuilder;
import io.grpc.ServerCredentials;
import io.grpc.netty.shaded.io.grpc.netty.GrpcSslContexts;
import io.grpc.netty.shaded.io.grpc.netty.NettyServerBuilder;
import io.grpc.netty.shaded.io.netty.handler.ssl.ClientAuth;
import io.grpc.netty.shaded.io.netty.handler.ssl.SslContext;
import io.grpc.netty.shaded.io.netty.handler.ssl.SslContextBuilder;
import io.grpc.netty.shaded.io.netty.handler.ssl.SslProvider;

public class ServerMain {

    public static final int DEFAULT_PORT = 6254;
    public static final int DEFAULT_SLOT = 1;

    public static void main(String[] args) {
        try {
            File serverWorkingDirectory = null;
            boolean secure = false;
            int port = DEFAULT_PORT;
            int slot = DEFAULT_SLOT;
            
            if (args.length > 0) {
                serverWorkingDirectory = new File(args[0]);
                serverWorkingDirectory.mkdirs();
            }
            
            if (args.length > 1) {
                port = Integer.parseInt(args[1]);
            }
            
            if (args.length > 2) {
                secure = "-secure".equalsIgnoreCase(args[2]);
            }
            
            CatenaServer catenaServer = new CatenaServer();
            MyCatenaDevice device = new MyCatenaDevice(catenaServer, slot, serverWorkingDirectory);
            catenaServer.addDevice(slot, device);
            device.start();
            
            Server server = null;
            ServerBuilder<?> serverBuilder = null;
            if (secure) {
                serverBuilder = createSecureServer(port, serverWorkingDirectory);
            }
            else
            {
                serverBuilder = createInsecureServer(port);
            }
            
            server = serverBuilder
                    .addService(catenaServer) 
                    .build().start();
            System.out.println("SERVER: Server started.");
            server.awaitTermination();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    protected static ServerBuilder<?> createInsecureServer(int port)
    {
        System.out.println("Insecure Server Requested");
        ServerBuilder<?> serverBuilder;
        ServerCredentials credentials = InsecureServerCredentials.create(); 
        serverBuilder = Grpc.newServerBuilderForPort(port, credentials);
        return serverBuilder;
    }

    protected static ServerBuilder<?> createSecureServer(int port, File serverWorkingDirectory) throws SSLException
    {
        System.out.println("Secure Server Requested");
        File certChain = null;
        File certKey = null;
        
        if (serverWorkingDirectory != null) {
            certChain = new File(serverWorkingDirectory, "server.pem");
            certKey = new File(serverWorkingDirectory, "server-key.pem");
        }

        if (certChain != null && certChain.exists() && certKey != null && certKey.exists())
        {
            SslContext sslContext = GrpcSslContexts.configure(SslContextBuilder.forServer(certChain, certKey))
                    .sslProvider(SslProvider.OPENSSL)
                    .clientAuth(ClientAuth.NONE) // Adjust as needed
                    .build();
      
            // Create and start gRPC server
            return NettyServerBuilder.forPort(port)
                    .sslContext(sslContext);
        }
        throw new IllegalArgumentException("Secure server requested but no certificate found");
    }
}
