package com.rossvideo.catena.command;

import io.grpc.stub.StreamObserver;

public class BidirectionalDelegatingStreamObserver<C, D> implements StreamObserver<C>
{
    private StreamObserver<C> delegate;
    private StreamObserver<D> responseStream;
    private BidirectionalStreamObserverFactory<C, D> factory;
    
    public BidirectionalDelegatingStreamObserver(BidirectionalStreamObserverFactory<C, D> factory, StreamObserver<D> responseStream)
    {
        this.factory = factory;
        this.responseStream = responseStream;
    }

    @Override
    public void onCompleted()
    {
        if (delegate != null)
        {
            delegate.onCompleted();
        }
    }

    @Override
    public void onError(Throwable arg0)
    {
        if (delegate != null)
        {
            delegate.onError(arg0);
        }
    }

    @Override
    public void onNext(C arg0)
    {
        if (delegate == null)
        {
            delegate = factory.createStreamObserver(arg0, responseStream);
        }
        
        if (delegate != null)
        {
            delegate.onNext(arg0);
        }
    }

}
