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
	"bytes"
	"encoding/json"
	"fmt"
)

// orderedObj is a JSON object that preserves key insertion/parse order.
// Go's map[string]any loses ordering; this type retains it so that
// protojson's proto-field-number ordering survives a round-trip.
type orderedObj struct {
	pairs []kv
}

type kv struct {
	key string
	val any
}

// get returns the value for key and whether it exists.
func (o *orderedObj) get(key string) (any, bool) {
	for _, p := range o.pairs {
		if p.key == key {
			return p.val, true
		}
	}
	return nil, false
}

// setDefault appends key:value only if key is not already present.
func (o *orderedObj) setDefault(key string, value any) {
	if _, exists := o.get(key); !exists {
		o.pairs = append(o.pairs, kv{key, value})
	}
}

// set overwrites an existing key or appends a new one.
func (o *orderedObj) set(key string, value any) {
	for i, p := range o.pairs {
		if p.key == key {
			o.pairs[i].val = value
			return
		}
	}
	o.pairs = append(o.pairs, kv{key, value})
}

// setFirst overwrites an existing key in place, or prepends it at position 0.
func (o *orderedObj) setFirst(key string, value any) {
	for i, p := range o.pairs {
		if p.key == key {
			o.pairs[i].val = value
			return
		}
	}
	o.pairs = append([]kv{{key, value}}, o.pairs...)
}

// MarshalJSON emits keys in their original order.
func (o *orderedObj) MarshalJSON() ([]byte, error) {
	var buf bytes.Buffer
	buf.WriteByte('{')
	for i, p := range o.pairs {
		if i > 0 {
			buf.WriteByte(',')
		}
		k, _ := json.Marshal(p.key)
		buf.Write(k)
		buf.WriteByte(':')
		v, err := marshalOrderedValue(p.val)
		if err != nil {
			return nil, err
		}
		buf.Write(v)
	}
	buf.WriteByte('}')
	return buf.Bytes(), nil
}

func marshalOrderedValue(v any) ([]byte, error) {
	switch val := v.(type) {
	case *orderedObj:
		return val.MarshalJSON()
	case []any:
		var buf bytes.Buffer
		buf.WriteByte('[')
		for i, item := range val {
			if i > 0 {
				buf.WriteByte(',')
			}
			b, err := marshalOrderedValue(item)
			if err != nil {
				return nil, err
			}
			buf.Write(b)
		}
		buf.WriteByte(']')
		return buf.Bytes(), nil
	default:
		return json.Marshal(v)
	}
}

// parseOrderedJSON parses a JSON object from bytes, preserving key order.
func parseOrderedJSON(data []byte) (*orderedObj, error) {
	dec := json.NewDecoder(bytes.NewReader(data))
	dec.UseNumber()
	return parseOrderedObject(dec)
}

func parseOrderedObject(dec *json.Decoder) (*orderedObj, error) {
	t, err := dec.Token()
	if err != nil {
		return nil, err
	}
	if d, ok := t.(json.Delim); !ok || d != '{' {
		return nil, fmt.Errorf("expected '{', got %v", t)
	}

	obj := &orderedObj{}
	for dec.More() {
		keyTok, err := dec.Token()
		if err != nil {
			return nil, err
		}
		key := keyTok.(string)

		val, err := parseOrderedValue(dec)
		if err != nil {
			return nil, fmt.Errorf("key %q: %w", key, err)
		}
		obj.pairs = append(obj.pairs, kv{key, val})
	}
	dec.Token() // consume closing '}'
	return obj, nil
}

func parseOrderedValue(dec *json.Decoder) (any, error) {
	t, err := dec.Token()
	if err != nil {
		return nil, err
	}

	switch v := t.(type) {
	case json.Delim:
		switch v {
		case '{':
			obj := &orderedObj{}
			for dec.More() {
				keyTok, err := dec.Token()
				if err != nil {
					return nil, err
				}
				key := keyTok.(string)
				val, err := parseOrderedValue(dec)
				if err != nil {
					return nil, fmt.Errorf("key %q: %w", key, err)
				}
				obj.pairs = append(obj.pairs, kv{key, val})
			}
			dec.Token() // consume '}'
			return obj, nil
		case '[':
			var arr []any
			for dec.More() {
				val, err := parseOrderedValue(dec)
				if err != nil {
					return nil, err
				}
				arr = append(arr, val)
			}
			dec.Token() // consume ']'
			return arr, nil
		}
	case json.Number:
		if i, err := v.Int64(); err == nil {
			return i, nil
		}
		f, err := v.Float64()
		if err != nil {
			return nil, err
		}
		return f, nil
	case string:
		return v, nil
	case bool:
		return v, nil
	case nil:
		return nil, nil
	}
	return nil, fmt.Errorf("unexpected token: %v", t)
}
