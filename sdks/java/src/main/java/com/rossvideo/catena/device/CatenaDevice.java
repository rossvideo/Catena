package com.rossvideo.catena.device;

import java.util.Map;

import com.google.protobuf.Empty;

import catena.core.device.DeviceComponent;
import catena.core.device.DeviceComponent.ComponentLanguagePack;
import catena.core.device.DeviceComponent.ComponentParam;
import catena.core.device.DeviceRequestPayload;
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
import catena.core.service.EmptyRequest;
import io.grpc.stub.StreamObserver;

public interface CatenaDevice
{
    public default StreamObserver<ExecuteCommandPayload> executeCommand(ExecuteCommandPayload firstMessage, StreamObserver<CommandResponse> responseStream, Map<String, Object> claims) { return null; };
    public default void deviceRequest(DeviceRequestPayload request, StreamObserver<DeviceComponent> responseObserver, Map<String, Object> claims) { responseObserver.onError(new UnsupportedOperationException("Not supported")); };
    public default void setValue(SetValuePayload request, StreamObserver<Empty> responseObserver, Map<String, Object> claims) { responseObserver.onError(new UnsupportedOperationException("Not supported")); };
    public default void getValue(GetValuePayload request, StreamObserver<Value> responseObserver, Map<String, Object> claims) { responseObserver.onError(new UnsupportedOperationException("Not supported")); };
    public default void getParam(GetParamPayload request, StreamObserver<ComponentParam> responseObserver, Map<String, Object> claims) { responseObserver.onError(new UnsupportedOperationException("Not supported")); };
    public default void externalObjectRequest(ExternalObjectRequestPayload request, StreamObserver<ExternalObjectPayload> responseObserver, Map<String, Object> claims) { responseObserver.onError(new UnsupportedOperationException("Not supported")); };
    public default void basicParamInfoRequest(BasicParamInfoRequestPayload request, StreamObserver<BasicParamInfoResponse> responseObserver, Map<String, Object> claims) { responseObserver.onError(new UnsupportedOperationException("Not supported")); };
    public default void multiSetValue(MultiSetValuePayload request, StreamObserver<Empty> responseObserver, Map<String, Object> claims) { responseObserver.onError(new UnsupportedOperationException("Not supported")); };
    public default void updateSubscriptions(UpdateSubscriptionsPayload request, StreamObserver<ComponentParam> responseObserver, Map<String, Object> claims) { responseObserver.onError(new UnsupportedOperationException("Not supported")); };
    public default void addLanguage(AddLanguagePayload request, StreamObserver<Empty> responseObserver, Map<String, Object> claims) { responseObserver.onError(new UnsupportedOperationException("Not supported")); };
    public default void languagePackRequest(LanguagePackRequestPayload request, StreamObserver<ComponentLanguagePack> responseObserver, Map<String, Object> claims) { responseObserver.onError(new UnsupportedOperationException("Not supported")); };
    public default void listLanguages(EmptyRequest request, StreamObserver<LanguageList> responseObserver, Map<String, Object> claims) { responseObserver.onError(new UnsupportedOperationException("Not supported")); };
}
