package com.rossvideo.catena.command;

import io.grpc.stub.StreamObserver;

public interface BidirectionalStreamObserverFactory<C, D>
{
    public StreamObserver<C> createStreamObserver(C firstMessage, StreamObserver<D> responseStream);
}
