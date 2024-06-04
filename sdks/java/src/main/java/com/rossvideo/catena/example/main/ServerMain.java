package com.rossvideo.catena.example.main;

import java.io.File;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Map;

import com.rossvideo.catena.device.CatenaServer;
import com.rossvideo.catena.device.JsonCatenaDevice;
import com.rossvideo.catena.example.device.MyCatenaDevice;
import com.rossvideo.catena.oauth.JwtOAuthValidationUtils;
import com.rossvideo.catena.oauth.OAuthConfig;
import com.rossvideo.catena.oauth.OAuthServerInterceptor;
import com.rossvideo.catena.oauth.SimpleOAuthConfig;
import com.rossvideo.catena.oauth.URLOAuthConfig;

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
            if (args.length == 1 && "-h".equals(args[0]))
            {
                printHelp();
                return;
            }
            
            File serverWorkingDirectory = null;
            
            Map<String, String> arguments = MainUtil.parseArguments(args);
            boolean secure = MainUtil.booleanFromMap(arguments, "secure", false);
            int port = MainUtil.intFromMap(arguments, "port", DEFAULT_PORT);
            int slot = MainUtil.intFromMap(arguments, "slot", DEFAULT_SLOT);
            if (arguments.containsKey("d")) {
                serverWorkingDirectory = new File(arguments.get("d"));
                serverWorkingDirectory.mkdirs();
            }
            
            CatenaServer catenaServer = new MyCatenaServer();
            
            String deviceDefinition = MainUtil.stringFromMap(arguments, "device", null);
            if (deviceDefinition != null)
            {
                JsonCatenaDevice device = new JsonCatenaDevice(catenaServer);
                if (serverWorkingDirectory != null)
                {
                    device.init(new File(serverWorkingDirectory, deviceDefinition));
                }
                else
                {
                    device.init(new File(deviceDefinition));
                }
                catenaServer.addDevice(slot, device);
                device.start();
            }
            else
            {
                MyCatenaDevice device = new MyCatenaDevice(catenaServer, slot, serverWorkingDirectory);
                catenaServer.addDevice(slot, device);
                device.start();
            }
            
            
            Server server = null;
            ServerBuilder<?> serverBuilder = null;
            if (secure) {
                serverBuilder = createSecureServer(port, serverWorkingDirectory, arguments);
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

    private static void printHelp()
    {
        System.out.println("Usage: java -jar catena-server.jar [options]");
        System.out.println("-port [port number]");
        System.out.println("-secure [true|false]");
        System.out.println("-d [working directory]");
        System.out.println("-slot [slot number]");
        System.out.println("-oauth [realm url]");
        System.out.println("-oauth-client [client id]");
    }

    protected static ServerBuilder<?> createInsecureServer(int port)
    {
        System.out.println("Insecure Server Requested");
        ServerBuilder<?> serverBuilder;
        ServerCredentials credentials = InsecureServerCredentials.create(); 
        serverBuilder = Grpc.newServerBuilderForPort(port, credentials);
        return serverBuilder;
    }

    protected static ServerBuilder<?> createSecureServer(int port, File serverWorkingDirectory, Map<String, String> arguments) throws MalformedURLException, IOException
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
      
            OAuthConfig config = getOAuthConfig(arguments);
            
            // Create and start gRPC server
            return NettyServerBuilder.forPort(port)
                    .intercept(new OAuthServerInterceptor(new JwtOAuthValidationUtils(config)))
                    .sslContext(sslContext);
        }
        throw new IllegalArgumentException("Secure server requested but no certificate found");
    }

    private static OAuthConfig getOAuthConfig(Map<String, String> arguments) throws MalformedURLException, IOException
    {
        String oauthRealm = MainUtil.stringFromMap(arguments, "oauth", null);
        String oauthClient = MainUtil.stringFromMap(arguments, "oauth-client", null);
        
        if (oauthRealm == null && oauthClient == null)
        {
            return new SimpleOAuthConfig();
        }
        
        URLOAuthConfig config = new URLOAuthConfig();
        config.initFrom(new URL(oauthRealm), oauthClient, true);
        return config;
    }
}
