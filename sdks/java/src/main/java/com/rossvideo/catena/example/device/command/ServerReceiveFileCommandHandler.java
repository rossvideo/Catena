package com.rossvideo.catena.example.device.command;

import java.io.File;
import java.io.IOException;

import com.rossvideo.catena.datapayload.FilePayloadReceiver;

import catena.core.parameter.CommandResponse;
import catena.core.parameter.DataPayload;
import catena.core.parameter.ExecuteCommandPayload;
import catena.core.parameter.Value;
import io.grpc.stub.StreamObserver;

// public class ServerReceiveFileCommandHandler implements StreamObserver<ExecuteCommandPayload>
// {
//     private final StreamObserver<CommandResponse> responseObserver;  
//     private FilePayloadReceiver filePayloadReceiver;
    
//     public ServerReceiveFileCommandHandler(File workingDirectory, StreamObserver<CommandResponse> responseObserver)
//     {
//         this.responseObserver = responseObserver;
//         filePayloadReceiver = new FilePayloadReceiver(workingDirectory, "new file");
//     }
    
//     @Override
//     public void onCompleted()
//     {
//         int fileCount = filePayloadReceiver.getFileCount();
//         int skipCount = filePayloadReceiver.getSkipCount();
//         File currentFile = filePayloadReceiver.getCurrentFile();
//         filePayloadReceiver.close();

//         String responseString = "Received " + fileCount + " file" + (fileCount == 1 ? "" : "s");
//         if (fileCount == 1)
//         {
//             responseString = "Received file " + currentFile.getName();
//         }
//         if (skipCount > 0)
//         {
//             responseString += "\nSkipped " + skipCount + " file" + (skipCount == 1 ? "" : "s");
//         }
//         responseObserver.onNext(CommandResponse.newBuilder().setResponse(Value.newBuilder().setStringValue(responseString)).build());
//         responseObserver.onCompleted();
//     }

//     @Override
//     public void onError(Throwable arg0)
//     {
//         arg0.printStackTrace();
//     }

//     @Override
//     public void onNext(ExecuteCommandPayload arg0)
//     {
//         Value value = arg0.getValue();
//         if (value == null || value.getDataPayload() == null)
//         {
//             responseObserver.onError(new IllegalArgumentException("No data payload provided."));
//             return;
//         }
        
//         try
//         {
//             DataPayload dataPayload = value.getDataPayload();
//             if (filePayloadReceiver.isNewFile(dataPayload))
//             {
//                 switch(filePayloadReceiver.init(dataPayload))
//                 {
//                     case PROCEED:
//                         responseObserver.onNext(CommandResponse.newBuilder().setProceed(true).build());
//                         break;
//                     case SKIP:
//                         responseObserver.onNext(CommandResponse.newBuilder().setProceed(false).build());
//                         break;
//                     case NONE:
//                         break;
//                 }
                
//             }
//             else
//             {
//                 filePayloadReceiver.writePayload(dataPayload);
//             }
//         }
//         catch (IOException e)
//         {
//             responseObserver.onError(e);
//         }
//     }
// }
