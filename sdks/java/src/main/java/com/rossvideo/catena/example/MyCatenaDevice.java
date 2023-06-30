package com.rossvideo.catena.example;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import com.google.protobuf.Empty;
import com.rossvideo.catena.example.error.InvalidSlotNumberException;
import com.rossvideo.catena.example.error.UnknownOidException;
import com.rossvideo.catena.example.error.WrongValueTypeException;

import catena.core.CatenaServiceGrpc.CatenaServiceImplBase;
import catena.core.Device;
import catena.core.DeviceComponent;
import catena.core.DeviceRequestPayload;
import catena.core.GetValuePayload;
import catena.core.Param;
import catena.core.ParamDescriptor;
import catena.core.ParamType;
import catena.core.ParamType.Type;
import catena.core.PolyglotText;
import catena.core.SetValuePayload;
import catena.core.Value;
import catena.core.Value.KindCase;
import io.grpc.stub.StreamObserver;

public class MyCatenaDevice extends CatenaServiceImplBase {

	public static final String INT_OID = "integer";
	public static final String FLOAT_OID = "float";
	
	private static final AtomicInteger deviceId = new AtomicInteger(1);
	
	private Value intValue = Value.newBuilder().setInt32Value(0).build();
	private Value floatValue = Value.newBuilder().setFloat32Value(0F).build();

	private final String deviceName = MyCatenaDevice.class.getSimpleName() + '-' + deviceId.getAndIncrement();
	private final int slot;

	public MyCatenaDevice(int slot) {
		this.slot = slot;
	}

	@Override
	public void deviceRequest(DeviceRequestPayload request, StreamObserver<DeviceComponent> responseObserver) {
		int slot = request.getSlot();
		System.out.println("SERVER: deviceRequest request for slot: " + slot);
		if (this.slot != slot) {
			responseObserver.onError(new InvalidSlotNumberException(slot));
			return;
		}
		
		Device device = Device.newBuilder().setSlot(this.slot).putAllParams(buildAllParamDescriptors()).build();
		DeviceComponent deviceComponent = DeviceComponent.newBuilder().setDevice(device).build();
		responseObserver.onNext(deviceComponent);
		responseObserver.onCompleted();
	}
	
	private Map<String, ParamDescriptor> buildAllParamDescriptors() {
		Map<String, ParamDescriptor> parameters = new HashMap<>();
		parameters.put(FLOAT_OID, buildParamDescriptor(FLOAT_OID, "float parameter", Type.FLOAT32, false, 1, floatValue));
		parameters.put(INT_OID, buildParamDescriptor(INT_OID, "int parameter", Type.INT32, false, 1, intValue));
		return parameters;
	}
	
	private ParamDescriptor buildParamDescriptor(String oid, String name, Type type, boolean readonly, int precision, Value value) {
		Param param = Param.newBuilder()
				.setFqoid(deviceName + '.' + oid)
				.setName(PolyglotText.newBuilder().setMonoglot(name).build())
				.setType(ParamType.newBuilder().setType(type).build())
				.setReadOnly(readonly)
				.setPrecision(precision)
				.setValue(value)
				.build();
		return ParamDescriptor.newBuilder().setParam(param).build();
	}
	
	@Override
	public void setValue(SetValuePayload request, StreamObserver<Empty> responseObserver) {
		System.out.println("SERVER: setValue request:");
		System.out.println("\toid:   " + request.getOid());
		System.out.println("\tslot:  " + request.getSlot());
		System.out.println("\tvalue: " + request.getValue());
		int slot = request.getSlot();
		if (this.slot != slot) {
			responseObserver.onError(new InvalidSlotNumberException(slot));
			return;
		}

		String oid = request.getOid();
		Value value = request.getValue();
		KindCase valueKindCase = value.getKindCase();
		switch (oid) {
			case INT_OID:
				if (valueKindCase != KindCase.INT32_VALUE) {
					responseObserver.onError(new WrongValueTypeException(oid, valueKindCase, KindCase.INT32_VALUE));
					return;
				}
				intValue = value;
				break;
			case FLOAT_OID:
				if (valueKindCase != KindCase.FLOAT32_VALUE) {
					responseObserver.onError(new WrongValueTypeException(oid, valueKindCase, KindCase.FLOAT32_VALUE));
					return;
				}
				floatValue = value;
				break;
			default:
				responseObserver.onError(new UnknownOidException(oid));
				return;
		}

		responseObserver.onNext(Empty.getDefaultInstance());
		responseObserver.onCompleted();
	}

	@Override
	public void getValue(GetValuePayload request, StreamObserver<Value> responseObserver) {
		System.out.println("SERVER: getValue request:");
		System.out.println("\toid:  " + request.getOid());
		System.out.println("\tslot: " + request.getSlot());
		System.out.println();
		int slot = request.getSlot();
		if (this.slot != slot) {
			responseObserver.onError(new InvalidSlotNumberException(slot));
			return;
		}

		Value value;
		String oid = request.getOid();
		switch (oid) {
		case INT_OID:
			value = intValue;
			break;
		case FLOAT_OID:
			value = floatValue;
			break;
		default:
			responseObserver.onError(new UnknownOidException(oid));
			return;
		}

		responseObserver.onNext(value);
		responseObserver.onCompleted();
	}

}
