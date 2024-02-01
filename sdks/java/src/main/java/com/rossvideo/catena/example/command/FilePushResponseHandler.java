package com.rossvideo.catena.example.command;

import java.io.File;
import java.io.IOException;

import com.rossvideo.catena.datapayload.DataPayloadBuilder;

import catena.core.parameter.CommandResponse;
import catena.core.parameter.DataPayload;
import catena.core.parameter.ExecuteCommandPayload;
import catena.core.parameter.Value;
import io.grpc.stub.StreamObserver;

public class FilePushResponseHandler extends CommandResponseHandler
{
    private DataPayloadBuilder dataPayloadBuilder;
    private StreamObserver<ExecuteCommandPayload> txStream;
    private boolean complete = false;
    private String oid;
    private Object notifier;
    private File[] files;
    private int fileIndex = 0;

    public FilePushResponseHandler(int slot, String oid, File files[], Object notifier)
    {
        this.files = files;
        this.notifier = notifier;
        this.oid = oid;
    }
    
    public boolean isComplete()
    {
        return complete;
    }
    
    private void release()
    {
        if (notifier != null)
        {
            synchronized (notifier)
            {
                notifier.notifyAll();
            }
        }
    }

    @Override
    public void onCompleted()
    {
        complete = true;
        release();    
    }

    @Override
    public void onError(Throwable arg0)
    {
        complete = true;
        release();
    }
    
    @Override
    public void onNext(CommandResponse arg0)
    {
        if (!complete)
        {
            try
            {
                if (arg0.getProceed())
                {
                    while (dataPayloadBuilder.hasNext())
                    {
                        txStream.onNext(ExecuteCommandPayload.newBuilder().setValue(Value.newBuilder().setDataPayload(dataPayloadBuilder.buildNext())).build());
                    }
                }
                complete = !sendNext();
                if (complete)
                {
                    txStream.onCompleted();
                }
            }
            catch (IOException e)
            {
                txStream.onError(e);
                complete = true;
            }
        }
        else
        {
            super.onNext(arg0);
        }
    }

    protected boolean sendNext() throws IOException
    {
        if (dataPayloadBuilder != null)
        {
            dataPayloadBuilder.close();
        }
        
        if (fileIndex < files.length)
        {
            dataPayloadBuilder = new DataPayloadBuilder(files[fileIndex], null);      
            fileIndex++;
            
            dataPayloadBuilder.calculateDigest();
            DataPayload firstPayload = dataPayloadBuilder.buildNext();
            txStream.onNext(ExecuteCommandPayload.newBuilder().setOid(oid).setValue(Value.newBuilder().setDataPayload(firstPayload)).build());
            return true;
        }
        
        return false;
    }
    
    public void setTxStream(StreamObserver<ExecuteCommandPayload> txStream) throws IOException
    {
        this.txStream = txStream;
        sendNext();
    }

}
