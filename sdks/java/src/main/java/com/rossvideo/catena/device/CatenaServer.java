/*
 * Copyright 2025 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file CatenaServer.java
 * @brief Implements the Catena Server
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @author Previous Co-ops
 * @date 2025-10-21
 * @copyright Copyright © 2025 Ross Video Ltd
 */

package com.rossvideo.catena.device;

import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

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
import catena.core.language.Slot;
import catena.core.parameter.ParamInfoRequestPayload;
import catena.core.parameter.ParamInfoResponse;
import catena.core.parameter.CommandResponse;
import catena.core.parameter.ExecuteCommandPayload;
import catena.core.parameter.GetParamPayload;
import catena.core.parameter.GetValuePayload;
import catena.core.parameter.MultiSetValuePayload;
import catena.core.parameter.SetValuePayload;
import catena.core.parameter.SingleSetValuePayload;
import catena.core.parameter.UpdateSubscriptionsPayload;
import catena.core.parameter.Value;
import catena.core.parameter.Empty;
import catena.core.service.CatenaServiceGrpc.CatenaServiceImplBase;
import io.grpc.Context;
import io.grpc.stub.ServerCallStreamObserver;
import io.grpc.stub.StreamObserver;

public class CatenaServer extends CatenaServiceImplBase
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
    public void executeCommand(ExecuteCommandPayload request, StreamObserver<CommandResponse> responseObserver) {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            device.executeCommand(request, responseObserver, getClaims());
        }
        else
        {
            notifyNoDeviceAtSlot(responseObserver, request.getSlot());
        }
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
    public void setValue(SingleSetValuePayload request, StreamObserver<Empty> responseObserver)
    {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            device.setValue(request.getValue(), responseObserver, getClaims());
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
    public void paramInfoRequest(ParamInfoRequestPayload request, StreamObserver<ParamInfoResponse> responseObserver)
    {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            device.paramInfoRequest(request, responseObserver, getClaims());
        }
        else
        {
            notifyNoDeviceAtSlot(responseObserver, request.getSlot());
        }
    }

    @Override
    public void multiSetValue(MultiSetValuePayload request, StreamObserver<Empty> responseObserver)
    {
        CatenaDevice device = getDevice(request.getSlot());
        if (device != null)
        {
            //TODO implement add multiSetValue that passes the correct subset to each slot as a multi-set
            for (SetValuePayload setValuePayload : request.getValuesList()) {
                device.setValue(setValuePayload, responseObserver, getClaims());
            }
        }
        else
        {
            notifyNoDeviceAtSlot(responseObserver, request.getSlot());
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
    public void addLanguage(AddLanguagePayload request, StreamObserver<Empty> responseObserver)
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
    public void listLanguages(Slot request, StreamObserver<LanguageList> responseObserver)
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

    @Override
    public void getPopulatedSlots(EmptyPayload request, StreamObserver<SlotList> responseObserver)
    {
        responseObserver.onNext(buildPopulatedSlots());
        responseObserver.onCompleted();
    }
    
}
