package com.rossvideo.catena.example.main;

import java.util.Map;

import com.rossvideo.catena.device.CatenaServer;

import catena.core.device.ConnectPayload;
import catena.core.device.PushUpdates;
import io.grpc.stub.StreamObserver;

public class MyCatenaServer extends CatenaServer
{

    @Override
    public void connect(ConnectPayload request, StreamObserver<PushUpdates> responseObserver)
    {
        Map<String, Object> claims = getClaims();
        if (claims != null)
        {
            System.out.println("User " + claims.get("name") + " connected");
        }
        super.connect(request, responseObserver);
    }

}
