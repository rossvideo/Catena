package com.rossvideo.catena.device;

import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

import com.google.protobuf.Empty;
import com.rossvideo.catena.command.BidirectionalDelegatingStreamObserver;
import com.rossvideo.catena.command.BidirectionalStreamObserverFactory;

import catena.core.device.ConnectPayload;
import catena.core.device.DeviceComponent;
import catena.core.device.DeviceComponent.ComponentLanguagePack;
import catena.core.device.DeviceComponent.ComponentParam;
import catena.core.device.DeviceRequestPayload;
import catena.core.device.PushUpdates;
import catena.core.device.SlotList;
import catena.core.externalobject.ExternalObjectPayload;
import catena.core.externalobject.ExternalObjectRequestPayload;
import catena.core.language.AddLanguagePayload;
import catena.core.language.LanguageList;
import catena.core.language.LanguagePackRequestPayload;
import catena.core.parameter.BasicParamInfoRequestPayload;
import catena.core.parameter.BasicParamInfoResponse;
import catena.core.parameter.CommandResponse;
import catena.core.parameter.ExecuteCommandPayload;
import catena.core.parameter.GetParamPayload;
import catena.core.parameter.GetValuePayload;
import catena.core.parameter.MultiSetValuePayload;
import catena.core.parameter.SetValuePayload;
import catena.core.parameter.UpdateSubscriptionsPayload;
import catena.core.parameter.Value;
import catena.core.service.CatenaServiceGrpc.CatenaServiceImplBase;
import catena.core.service.GeneralRequest;
import catena.core.service.GeneralResponse;
import io.grpc.Context;
import io.grpc.stub.ServerCallStreamObserver;
import io.grpc.stub.StreamObserver;

public class CatenaServer extends CatenaServiceImplBase implements BidirectionalStreamObserverFactory<ExecuteCommandPayload, CommandResponse>
{
    public static final Context.Key<Map<String, Object>> KEY_CLAIMS = Context.key("claims");
    
    private Set<StreamObserver<PushUpdates>> pushConnections = new LinkedHashSet<>();
    private Map<Integer, CatenaDevice> devices = new HashMap<>();
    
    protected Map<String, Object> getClaims()
    {
        Context context = Context.current();
        return KEY_CLAIMS.get(context);
    }
    
    public void addDevice(Integer slot, CatenaDevice device) {
        devices.put(slot, device);
        notifySlotAdded(slot);
    }
    
    private void notifySlotAdded(Integer slot)
    {
        notifyClients(PushUpdates.newBuilder()
                .setSlotsAdded(SlotList.newBuilder()
                        .addSlots(slot.intValue())
                        ).build());
    }

    public void removeDevice(Integer slot) {
        devices.remove(slot);
        notifySlotRemoved(slot);
    }
    
    private void notifySlotRemoved(Integer slot)
    {
        notifyClients(PushUpdates.newBuilder()
                .setSlotsRemoved(SlotList.newBuilder()
                        .addSlots(slot.intValue())
                        ).build());
    }
    
    private SlotList buildPopulatedSlots()
    {
        SlotList.Builder slotList = SlotList.newBuilder();
        for (Integer slot : devices.keySet())
        {
            slotList.addSlots(slot.intValue());
        }
        return slotList.build();
    }
    
    public void notifyClients(PushUpdates pushUpdates)
    {
        synchronized (pushConnections)
        {
            for (StreamObserver<PushUpdates> observer : pushConnections)
            {
                observer.onNext(pushUpdates);
            }
        }
    }

    @Override
    public StreamObserver<ExecuteCommandPayload> createStreamObserver(ExecuteCommandPayload firstMessage, StreamObserver<CommandResponse> responseStream)
    {
        CatenaDevice device = getDevice(firstMessage.getSlot());
        if (device != null)
        {
            return device.executeCommand(firstMessage, responseStream, getClaims());
        }
        return null;
    }
    
    private CatenaDevice getDevice(int slot)
    {
        return devices.get(slot);
    }

    protected void addConnection(StreamObserver<PushUpdates> responseObserver)
    {
        synchronized (pushConnections)
        {
            if (pushConnections.add(responseObserver) && responseObserver instanceof ServerCallStreamObserver)
            {
                ((ServerCallStreamObserver<PushUpdates>) responseObserver).setOnCancelHandler(() -> {
                    removeConnection(responseObserver);
                });
            }
        }
    }

    protected void removeConnection(StreamObserver<PushUpdates> responseObserver)
    {
        synchronized (pushConnections)
        {
            pushConnections.remove(responseObserver);
            responseObserver.onCompleted();
        }
    }
    
    private void notifyNoDeviceAtSlot(StreamObserver<?> responseObserver, int slot)
    {
        responseObserver.onError(new IllegalArgumentException("No device registered with slot: " + slot));
    }

    @Override
    public void connect(ConnectPayload request, StreamObserver<PushUpdates> responseObserver)
    {
        addConnection(responseObserver);
        SlotList populatedSlots = buildPopulatedSlots();
        responseObserver.onNext(PushUpdates.newBuilder().setSlotsAdded(populatedSlots).build());
    }
    
    @Override
    public StreamObserver<ExecuteCommandPayload> executeCommand(StreamObserver<CommandResponse> responseObserver) {
        return new BidirectionalDelegatingStreamObserver<ExecuteCommandPayload, CommandResponse>(this, responseObserver);
    }
    
    @Override
    public void deviceRequest(DeviceRequestPayload request, StreamObserver<DeviceComponent> responseObserver)
    {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            device.deviceRequest(request, responseObserver, getClaims());
        }
        else
        {
            notifyNoDeviceAtSlot(responseObserver, request.getSlot());
        }
    }

    @Override
    public void setValue(SetValuePayload request, StreamObserver<Empty> responseObserver)
    {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            device.setValue(request, responseObserver, getClaims());
        }
        else
        {
            notifyNoDeviceAtSlot(responseObserver, request.getSlot());
        }
    }

    @Override
    public void getValue(GetValuePayload request, StreamObserver<Value> responseObserver)
    {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            device.getValue(request, responseObserver, getClaims());
        }
        else
        {
            notifyNoDeviceAtSlot(responseObserver, request.getSlot());
        }
    }

    @Override
    public void getParam(GetParamPayload request, StreamObserver<ComponentParam> responseObserver)
    {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            device.getParam(request, responseObserver, getClaims());
        }
        else
        {
            notifyNoDeviceAtSlot(responseObserver, request.getSlot());
        }
    }

    @Override
    public void externalObjectRequest(ExternalObjectRequestPayload request, StreamObserver<ExternalObjectPayload> responseObserver)
    {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            device.externalObjectRequest(request, responseObserver, getClaims());
        }
        else
        {
            notifyNoDeviceAtSlot(responseObserver, request.getSlot());
        }
    }

    @Override
    public void basicParamInfoRequest(BasicParamInfoRequestPayload request, StreamObserver<BasicParamInfoResponse> responseObserver)
    {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            device.basicParamInfoRequest(request, responseObserver, getClaims());
        }
        else
        {
            notifyNoDeviceAtSlot(responseObserver, request.getSlot());
        }
    }

    @Override
    public void multiSetValue(MultiSetValuePayload request, StreamObserver<Empty> responseObserver)
    {
        //TODO implement add multiSetValue that passes the correct subset to each slot as a multi-set
        for (SetValuePayload setValuePayload : request.getValuesList()) {
            setValue(setValuePayload, responseObserver);
        }
    }

    @Override
    public void updateSubscriptions(UpdateSubscriptionsPayload request, StreamObserver<ComponentParam> responseObserver)
    {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            device.updateSubscriptions(request, responseObserver, getClaims());
        }
        else
        {
            notifyNoDeviceAtSlot(responseObserver, request.getSlot());
        }
    }

    @Override
    public void addLanguage(AddLanguagePayload request, StreamObserver<GeneralResponse> responseObserver)
    {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            device.addLanguage(request, responseObserver, getClaims());
        }
        else
        {
            notifyNoDeviceAtSlot(responseObserver, request.getSlot());
        }
    }

    @Override
    public void languagePackRequest(LanguagePackRequestPayload request, StreamObserver<ComponentLanguagePack> responseObserver)
    {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            device.languagePackRequest(request, responseObserver, getClaims());
        }
        else
        {
            notifyNoDeviceAtSlot(responseObserver, request.getSlot());
        }
    }

    @Override
    public void listLanguages(GeneralRequest request, StreamObserver<LanguageList> responseObserver)
    {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            device.listLanguages(request, responseObserver, getClaims());
        }
        else
        {
            notifyNoDeviceAtSlot(responseObserver, request.getSlot());
        }
    }
    
    
}
