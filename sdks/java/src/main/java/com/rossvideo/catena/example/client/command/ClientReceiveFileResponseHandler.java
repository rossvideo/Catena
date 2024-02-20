package com.rossvideo.catena.example.client.command;

import java.io.File;
import java.io.IOException;

import com.google.protobuf.Empty;
import com.rossvideo.catena.datapayload.AbstractFilePayloadReceiver.ProceedResponse;
import com.rossvideo.catena.datapayload.FilePayloadReceiver;

import catena.core.parameter.CommandResponse;
import catena.core.parameter.DataPayload;
import catena.core.parameter.ExecuteCommandPayload;
import catena.core.parameter.Value;
import io.grpc.stub.StreamObserver;

public class ClientReceiveFileResponseHandler implements StreamObserver<CommandResponse>
{
    private File workingDirectory;
    private FilePayloadReceiver filePayloadReceiver;
    private StreamObserver<ExecuteCommandPayload> txStream;
    private String oid;
    private Object notifier;
    private int slotNumber;
    
    public ClientReceiveFileResponseHandler(String oid, int slotNumber, File workingDirectory, Object notifier)
    {
        this.oid = oid;
        this.slotNumber = slotNumber;
        this.workingDirectory = workingDirectory;
        this.notifier = notifier;
    }
    
    private void release()
    {
        if (filePayloadReceiver != null)
        {
            filePayloadReceiver.close();
        }
        
        if (filePayloadReceiver.getSkipCount() > 0)
        {
            System.out.println("CLIENT SKIPPED " + filePayloadReceiver.getSkipCount() + " files.");            
        }
        else
        {
            System.out.println("CLIENT RECEIVED " + filePayloadReceiver.getCurrentFile());
        }

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
        release();
    }

    @Override
    public void onError(Throwable arg0)
    {
        release();
    }
    
    public void setTxStream(StreamObserver<ExecuteCommandPayload> txStream)
    {
        this.txStream = txStream;
        txStream.onNext(ExecuteCommandPayload.newBuilder().setOid(oid).setSlot(slotNumber).setValue(Value.newBuilder().setEmptyValue(Empty.getDefaultInstance()).build()).build());
    }

    @Override
    public void onNext(CommandResponse arg0)
    {
        if (filePayloadReceiver == null)
        {
            filePayloadReceiver = new FilePayloadReceiver(workingDirectory, null);
        }
        
        Value response = arg0.getResponse();
        if (response != null && response.hasDataPayload())
        {
            try
            {
                DataPayload payload = response.getDataPayload();
                if (filePayloadReceiver.isNewFile(payload))
                {
                    ProceedResponse resp = filePayloadReceiver.init(payload);
                    switch (resp)
                    {
                        case NONE:
                            break;
                        case PROCEED:
                            txStream.onNext(ExecuteCommandPayload.newBuilder().setProceed(true).build());
                            break;
                        case SKIP:
                            txStream.onNext(ExecuteCommandPayload.newBuilder().setProceed(false).build());
                            break;
                        default:
                            break;
                        
                    }
                }
                else
                {
                    filePayloadReceiver.writePayload(payload);
                }
            }
            catch (IOException e)
            {
                txStream.onError(e);
                filePayloadReceiver.close();
            }
        }
    }

}
