package com.rossvideo.catena.command;

import io.grpc.stub.StreamObserver;

public class DelegatingStreamObserver<C> implements StreamObserver<C>
{
    private StreamObserver<C> delegate;
    private StreamObserverFactory<C> factory;
    
    public DelegatingStreamObserver(StreamObserverFactory<C> factory)
    {
        this.factory = factory;
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
            delegate = factory.createStreamObserver(arg0);
        }
        
        if (delegate != null)
        {
            delegate.onNext(arg0);
        }
    }

}
