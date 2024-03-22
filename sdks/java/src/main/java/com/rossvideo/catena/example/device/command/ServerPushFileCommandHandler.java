package com.rossvideo.catena.example.device.command;

import java.io.IOException;
import java.io.InputStream;

import com.rossvideo.catena.datapayload.DataPayloadBuilder;

import catena.core.parameter.CommandResponse;
import catena.core.parameter.ExecuteCommandPayload;
import catena.core.parameter.Value;
import io.grpc.stub.StreamObserver;

public class ServerPushFileCommandHandler implements StreamObserver<ExecuteCommandPayload>
{
    private final StreamObserver<CommandResponse> responseStream;
    private DataPayloadBuilder payloadBuilder;
    private boolean waiting = false;

    public ServerPushFileCommandHandler(StreamObserver<CommandResponse> responseStream)
    {
        this.responseStream = responseStream;
    }

    protected InputStream createInputStream(String resource) throws IOException
    {
        return ServerPushFileCommandHandler.class.getResourceAsStream("files/" + resource);
    }
    
    protected void close()
    {
        if (payloadBuilder != null)
        {
            payloadBuilder.close();
            payloadBuilder = null;
        }
    }
    
    @Override
    public void onCompleted()
    {
        close();
    }

    @Override
    public void onError(Throwable arg0)
    {
        close();
    }
    
    private void sendNext() throws IOException
    {
        responseStream.onNext(CommandResponse.newBuilder().setResponse(Value.newBuilder().setDataPayload(payloadBuilder.buildNext()).build()).build());
    }

    @Override
    public void onNext(ExecuteCommandPayload arg0)
    {
        if (payloadBuilder == null)
        {
            try 
            {
                String resource = "sample-file.jpg";
                payloadBuilder = new DataPayloadBuilder(createInputStream(resource), resource, "image/jpeg");
                payloadBuilder.calculateDigest();
                waiting = true;
                sendNext();
            }
            catch (IOException e)
            {
                close();
                responseStream.onError(e);
            }
        }
        else if (waiting)
        {
            if (arg0.getProceed())
            {
                try
                {
                    while (payloadBuilder.hasNext())
                    {
                        sendNext();
                    }
                }
                catch (IOException e)
                {
                    close();
                    responseStream.onError(e);
                    return;
                }
            }
            
            close();
            responseStream.onCompleted();
        }
    }

}
