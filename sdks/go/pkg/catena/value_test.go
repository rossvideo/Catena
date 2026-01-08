package catena

import (
	"testing"

	"github.com/rossvideo/catena/build/go/protos"
)

func assertNotPanics(t *testing.T, f func()) {
	t.Helper()
	defer func() {
		if r := recover(); r != nil {
			t.Fatalf("unexpected panic: %v", r)
		}
	}()
	f()
}

func TestFromProto_NilDoesNotPanic(t *testing.T) {
	assertNotPanics(t, func() {
		if got := FromProto(nil); got != nil {
			t.Fatalf("expected nil, got %#v", got)
		}
	})
}

func TestFromProto_NestedNilValuesDoNotPanic(t *testing.T) {
	assertNotPanics(t, func() {
		pv := &protos.Value{
			Kind: &protos.Value_StructValue{
				StructValue: &protos.StructValue{
					Fields: map[string]*protos.Value{
						"supported":   {Kind: &protos.Value_Int32Value{Int32Value: 7}},
						"unsupported": nil, // e.g. ToProto returned nil
					},
				},
			},
		}

		got, ok := FromProto(pv).(map[string]any)
		if !ok {
			t.Fatalf("expected map[string]any, got %T", FromProto(pv))
		}
		if got["supported"] != int32(7) {
			t.Fatalf("expected supported=7, got %#v", got["supported"])
		}
		if got["unsupported"] != nil {
			t.Fatalf("expected unsupported=nil, got %#v", got["unsupported"])
		}
	})
}

func TestFromProto_StructArray_AllowsNilStructEntries(t *testing.T) {
	assertNotPanics(t, func() {
		pv := &protos.Value{
			Kind: &protos.Value_StructArrayValues{
				StructArrayValues: &protos.StructList{
					StructValues: []*protos.StructValue{
						nil,
						{Fields: map[string]*protos.Value{"x": nil}},
					},
				},
			},
		}

		got, ok := FromProto(pv).([]map[string]any)
		if !ok {
			t.Fatalf("expected []map[string]any, got %T", FromProto(pv))
		}
		if len(got) != 2 {
			t.Fatalf("expected len=2, got %d", len(got))
		}
		if got[0] == nil || len(got[0]) != 0 {
			t.Fatalf("expected first entry to be empty map, got %#v", got[0])
		}
		if got[1]["x"] != nil {
			t.Fatalf("expected got[1][\"x\"]=nil, got %#v", got[1]["x"])
		}
	})
}

func TestFromProto_StructVariant_NilValueDoesNotPanic(t *testing.T) {
	assertNotPanics(t, func() {
		pv := &protos.Value{
			Kind: &protos.Value_StructVariantValue{
				StructVariantValue: &protos.StructVariantValue{
					StructVariantType: "t",
					Value:             nil,
				},
			},
		}

		got, ok := FromProto(pv).(StructVariantValue)
		if !ok {
			t.Fatalf("expected StructVariantValue, got %T", FromProto(pv))
		}
		if got.StructVariantType != "t" {
			t.Fatalf("expected type=t, got %q", got.StructVariantType)
		}
		if got.Value != nil {
			t.Fatalf("expected value=nil, got %#v", got.Value)
		}
	})
}

func TestFromProto_StructVariantArray_AllowsNilEntries(t *testing.T) {
	assertNotPanics(t, func() {
		pv := &protos.Value{
			Kind: &protos.Value_StructVariantArrayValues{
				StructVariantArrayValues: &protos.StructVariantList{
					StructVariants: []*protos.StructVariantValue{
						nil,
						{StructVariantType: "t", Value: nil},
					},
				},
			},
		}

		got, ok := FromProto(pv).([]StructVariantValue)
		if !ok {
			t.Fatalf("expected []StructVariantValue, got %T", FromProto(pv))
		}
		if len(got) != 2 {
			t.Fatalf("expected len=2, got %d", len(got))
		}
		if got[0].StructVariantType != "" || got[0].Value != nil {
			t.Fatalf("expected zero-value element for nil entry, got %#v", got[0])
		}
		if got[1].StructVariantType != "t" || got[1].Value != nil {
			t.Fatalf("expected type=t and value=nil, got %#v", got[1])
		}
	})
}

func TestFromProto_ArrayKinds_NilListsDoNotPanic(t *testing.T) {
	assertNotPanics(t, func() {
		if got := FromProto(&protos.Value{Kind: &protos.Value_Int32ArrayValues{Int32ArrayValues: nil}}); got != nil {
			t.Fatalf("expected nil slice for int32 array, got %#v", got)
		}
		if got := FromProto(&protos.Value{Kind: &protos.Value_Float32ArrayValues{Float32ArrayValues: nil}}); got != nil {
			t.Fatalf("expected nil slice for float32 array, got %#v", got)
		}
		if got := FromProto(&protos.Value{Kind: &protos.Value_StringArrayValues{StringArrayValues: nil}}); got != nil {
			t.Fatalf("expected nil slice for string array, got %#v", got)
		}
	})
}
