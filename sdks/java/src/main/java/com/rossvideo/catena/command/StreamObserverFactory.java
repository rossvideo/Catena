package com.rossvideo.catena.command;

import io.grpc.stub.StreamObserver;

public interface StreamObserverFactory<C>
{
    public StreamObserver<C> createStreamObserver(C firstMessage);
}
