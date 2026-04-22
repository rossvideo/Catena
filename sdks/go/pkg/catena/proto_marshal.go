/*
 * Copyright 2026 Ross Video Ltd
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package catena

import (
	"sort"

	"google.golang.org/protobuf/reflect/protoreflect"
)

// protoToOrderedObj walks a proto message via reflection and builds an
// orderedObj directly, skipping the protojson→[]byte→parse round-trip.
// Fields are emitted in proto field-number order (matching protojson),
// and only populated (non-default) fields are included (matching
// EmitUnpopulated: false). Map keys are sorted lexicographically.
func protoToOrderedObj(msg protoreflect.Message) *orderedObj {
	obj := &orderedObj{}
	msg.Range(func(fd protoreflect.FieldDescriptor, v protoreflect.Value) bool {
		obj.pairs = append(obj.pairs, kv{fd.TextName(), protoFieldToAny(fd, v)})
		return true
	})
	return obj
}

func protoFieldToAny(fd protoreflect.FieldDescriptor, v protoreflect.Value) any {
	if fd.IsMap() {
		return protoMapToOrderedObj(fd, v.Map())
	}
	if fd.IsList() {
		return protoListToSlice(fd, v.List())
	}
	return protoSingularToAny(fd, v)
}

func protoMapToOrderedObj(fd protoreflect.FieldDescriptor, m protoreflect.Map) *orderedObj {
	valueFd := fd.MapValue()
	type entry struct {
		key string
		val protoreflect.Value
	}
	entries := make([]entry, 0, m.Len())
	m.Range(func(k protoreflect.MapKey, v protoreflect.Value) bool {
		entries = append(entries, entry{k.String(), v})
		return true
	})
	sort.Slice(entries, func(i, j int) bool {
		return entries[i].key < entries[j].key
	})

	obj := &orderedObj{}
	for _, e := range entries {
		obj.pairs = append(obj.pairs, kv{e.key, protoSingularToAny(valueFd, e.val)})
	}
	return obj
}

func protoListToSlice(fd protoreflect.FieldDescriptor, list protoreflect.List) []any {
	out := make([]any, list.Len())
	for i := range out {
		out[i] = protoSingularToAny(fd, list.Get(i))
	}
	return out
}

// protoSingularToAny converts a single proto value to its Go equivalent.
// Uses float32 for FloatKind to preserve protojson-compatible precision
// when the value is later serialized with encoding/json.
func protoSingularToAny(fd protoreflect.FieldDescriptor, v protoreflect.Value) any {
	switch fd.Kind() {
	case protoreflect.MessageKind, protoreflect.GroupKind:
		return protoToOrderedObj(v.Message())
	case protoreflect.EnumKind:
		return string(fd.Enum().Values().ByNumber(v.Enum()).Name())
	case protoreflect.BoolKind:
		return v.Bool()
	case protoreflect.Int32Kind, protoreflect.Sint32Kind, protoreflect.Sfixed32Kind:
		return int32(v.Int())
	case protoreflect.Uint32Kind, protoreflect.Fixed32Kind:
		return uint32(v.Uint())
	case protoreflect.Int64Kind, protoreflect.Sint64Kind, protoreflect.Sfixed64Kind:
		return v.Int()
	case protoreflect.Uint64Kind, protoreflect.Fixed64Kind:
		return v.Uint()
	case protoreflect.FloatKind:
		return float32(v.Float())
	case protoreflect.DoubleKind:
		return v.Float()
	case protoreflect.StringKind:
		return v.String()
	case protoreflect.BytesKind:
		return v.Bytes()
	default:
		return v.Interface()
	}
}
