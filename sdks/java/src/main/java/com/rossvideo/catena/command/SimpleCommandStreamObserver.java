package com.rossvideo.catena.command;

import catena.core.parameter.CommandResponse;
import catena.core.parameter.ExecuteCommandPayload;
import io.grpc.stub.StreamObserver;

public class SimpleCommandStreamObserver implements StreamObserver<ExecuteCommandPayload>
{
    private StreamObserver<CommandResponse> responseObserver;
    private SimpleCommandHandler processor;

    public SimpleCommandStreamObserver(StreamObserver<CommandResponse> responseObserver, SimpleCommandHandler processor)
    {
        this.processor = processor;
        this.responseObserver = responseObserver;
    }

    @Override
    public void onCompleted()
    {
        //DO NOTHING
    }

    @Override
    public void onError(Throwable arg0)
    {
        //DO NOTHING
    }

    @Override
    public void onNext(ExecuteCommandPayload execution)
    {
        try
        {
            CommandResponse response = processor.processCommand(execution);
            if (response != null)
            {
                responseObserver.onNext(response);
            }
            responseObserver.onCompleted();
        }
        catch (Exception exception)
        {
            onError(exception);
        }
        
    }


    
}
