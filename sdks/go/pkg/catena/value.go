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
	"fmt"

	"github.com/rossvideo/catena/build/go/protos"
)

type CatenaValue struct {
	Value *protos.Value
}

// CatenaAsset represents a binary asset with HTTP headers for GetAsset responses
type CatenaAsset struct {
	ContentType        string
	ContentDisposition string
	Payload            DataPayload       // Use DataPayload instead of raw bytes
	CustomHeaders      map[string]string // Additional headers if needed
}

type StructVariantValue struct {
	StructVariantType string `json:"struct_variant_type"`
	Value             any    `json:"value"`
}

type EmptyValue struct{}

type UndefinedValue int32

type DataPayload struct {
	Metadata        map[string]string `json:"metadata,omitempty"`
	Digest          []byte            `json:"digest,omitempty"`
	PayloadEncoding string            `json:"payload_encoding,omitempty"` // "uncompressed", "gzip", or "deflate"
	URL             string            `json:"url,omitempty"`
	Payload         []byte            `json:"payload,omitempty"`
}

func ToCatenaValue(v any) (CatenaValue, error) {
	val, err := ToProto(v)
	if err != nil {
		return CatenaValue{}, fmt.Errorf("ToCatenaValue: %w", err)
	}
	return CatenaValue{Value: val}, nil
}

// NewCatenaAsset creates a CatenaAsset from binary data and metadata
func NewCatenaAsset(data []byte, contentType string) CatenaAsset {
	return CatenaAsset{
		ContentType: contentType,
		Payload: DataPayload{
			Payload:         data,
			PayloadEncoding: "uncompressed",
		},
	}
}

// NewCatenaAssetFromURL creates a CatenaAsset that references an external URL
func NewCatenaAssetFromURL(url string, contentType string) CatenaAsset {
	return CatenaAsset{
		ContentType: contentType,
		Payload: DataPayload{
			URL:             url,
			PayloadEncoding: "uncompressed",
		},
	}
}

// NewCatenaAssetFromPayload creates a CatenaAsset from an existing DataPayload
func NewCatenaAssetFromPayload(payload DataPayload, contentType string) CatenaAsset {
	return CatenaAsset{
		ContentType: contentType,
		Payload:     payload,
	}
}

// WithFilename sets the Content-Disposition header for download
func (ca CatenaAsset) WithFilename(filename string) CatenaAsset {
	ca.ContentDisposition = fmt.Sprintf("attachment; filename=%q", filename)
	return ca
}

// WithInlineFilename sets the Content-Disposition header for inline display
func (ca CatenaAsset) WithInlineFilename(filename string) CatenaAsset {
	ca.ContentDisposition = fmt.Sprintf("inline; filename=%q", filename)
	return ca
}

// WithCustomHeader adds a custom HTTP header
func (ca CatenaAsset) WithCustomHeader(key, value string) CatenaAsset {
	if ca.CustomHeaders == nil {
		ca.CustomHeaders = make(map[string]string)
	}
	ca.CustomHeaders[key] = value
	return ca
}

// WithCompression sets the payload encoding (gzip, deflate, or uncompressed)
func (ca CatenaAsset) WithCompression(encoding string) CatenaAsset {
	ca.Payload.PayloadEncoding = encoding
	return ca
}

// WithDigest sets the digest/checksum for the payload
func (ca CatenaAsset) WithDigest(digest []byte) CatenaAsset {
	ca.Payload.Digest = digest
	return ca
}

// WithMetadata adds metadata to the payload
func (ca CatenaAsset) WithMetadata(key, value string) CatenaAsset {
	if ca.Payload.Metadata == nil {
		ca.Payload.Metadata = make(map[string]string)
	}
	ca.Payload.Metadata[key] = value
	return ca
}

// IsURL returns true if this asset is a URL reference
func (ca CatenaAsset) IsURL() bool {
	return ca.Payload.URL != ""
}

// GetData returns the embedded payload data (nil if URL-based)
func (ca CatenaAsset) GetData() []byte {
	return ca.Payload.Payload
}

// GetURL returns the URL (empty string if embedded data)
func (ca CatenaAsset) GetURL() string {
	return ca.Payload.URL
}

// ContentLength returns the size of the payload (0 for URL-based assets)
func (ca CatenaAsset) ContentLength() int64 {
	if ca.Payload.Payload != nil {
		return int64(len(ca.Payload.Payload))
	}
	return 0
}

// ToProto converts native Go types to protos.Value
func ToProto(v any) (*protos.Value, error) {
	switch val := v.(type) {
	case UndefinedValue:
		return &protos.Value{Kind: &protos.Value_UndefinedValue{UndefinedValue: protos.UndefinedValue(val)}}, nil
	case EmptyValue:
		return &protos.Value{Kind: &protos.Value_EmptyValue{EmptyValue: &protos.Empty{}}}, nil
	case int32:
		return &protos.Value{Kind: &protos.Value_Int32Value{Int32Value: val}}, nil
	case float32:
		return &protos.Value{Kind: &protos.Value_Float32Value{Float32Value: val}}, nil
	case string:
		return &protos.Value{Kind: &protos.Value_StringValue{StringValue: val}}, nil
	case []int32:
		return &protos.Value{Kind: &protos.Value_Int32ArrayValues{Int32ArrayValues: &protos.Int32List{Ints: val}}}, nil
	case []float32:
		return &protos.Value{Kind: &protos.Value_Float32ArrayValues{Float32ArrayValues: &protos.Float32List{Floats: val}}}, nil
	case []string:
		return &protos.Value{Kind: &protos.Value_StringArrayValues{StringArrayValues: &protos.StringList{Strings: val}}}, nil
	case map[string]any:
		fields := make(map[string]*protos.Value)
		for k, v := range val {
			res, err := ToProto(v)
			if err != nil {
				return nil, fmt.Errorf("failed to convert map field %s: %w", k, err)
			}
			fields[k] = res
		}
		return &protos.Value{Kind: &protos.Value_StructValue{StructValue: &protos.StructValue{Fields: fields}}}, nil
	case []map[string]any:
		structArr := make([]*protos.StructValue, len(val))
		for i, m := range val {
			fields := make(map[string]*protos.Value)
			for k, v := range m {
				res, err := ToProto(v)
				if err != nil {
					return nil, fmt.Errorf("failed to convert map field %s: %w", k, err)
				}
				fields[k] = res
			}
			structArr[i] = &protos.StructValue{Fields: fields}
		}
		return &protos.Value{Kind: &protos.Value_StructArrayValues{StructArrayValues: &protos.StructList{StructValues: structArr}}}, nil
	case StructVariantValue:
		protoVal, err := ToProto(val.Value)
		if err != nil {
			return nil, fmt.Errorf("failed to convert StructVariantValue.Value: %w", err)
		}
		return &protos.Value{Kind: &protos.Value_StructVariantValue{StructVariantValue: &protos.StructVariantValue{
			StructVariantType: val.StructVariantType,
			Value:             protoVal,
		}}}, nil
	case []StructVariantValue:
		var structVariants []*protos.StructVariantValue
		for _, sv := range val {
			protoVal, err := ToProto(sv.Value)
			if err != nil {
				return nil, fmt.Errorf("failed to convert StructVariantValue.Value in array: %w", err)
			}
			structVariants = append(structVariants, &protos.StructVariantValue{
				StructVariantType: sv.StructVariantType,
				Value:             protoVal,
			})
		}
		return &protos.Value{Kind: &protos.Value_StructVariantArrayValues{StructVariantArrayValues: &protos.StructVariantList{
			StructVariants: structVariants,
		}}}, nil
	case DataPayload:
		// Convert payload encoding string to enum
		var encoding protos.DataPayload_PayloadEncoding
		switch val.PayloadEncoding {
		case "gzip":
			encoding = protos.DataPayload_GZIP
		case "deflate":
			encoding = protos.DataPayload_DEFLATE
		default:
			encoding = protos.DataPayload_UNCOMPRESSED
		}

		dp := &protos.DataPayload{
			Metadata:        val.Metadata,
			Digest:          val.Digest,
			PayloadEncoding: encoding,
		}

		// Set either URL or Payload based on which is provided
		if val.URL != "" && val.Payload == nil {
			dp.Kind = &protos.DataPayload_Url{Url: val.URL}
		} else if val.Payload != nil && val.URL == "" {
			dp.Kind = &protos.DataPayload_Payload{Payload: val.Payload}
		} else {
			return nil, fmt.Errorf("DataPayload must have either URL or Payload set, but not both")
		}

		return &protos.Value{Kind: &protos.Value_DataPayload{DataPayload: dp}}, nil
	default:
		return nil, fmt.Errorf("unsupported type: %T", v)
	}
}

// FromProto converts protos.Value to native Go types
func FromProto(pv *protos.Value) any {
	if pv == nil {
		return nil
	}
	switch pv.GetKind().(type) {
	case *protos.Value_UndefinedValue:
		return UndefinedValue(pv.GetUndefinedValue())
	case *protos.Value_EmptyValue:
		return EmptyValue{}
	case *protos.Value_Int32Value:
		return pv.GetInt32Value()
	case *protos.Value_Float32Value:
		return pv.GetFloat32Value()
	case *protos.Value_StringValue:
		return pv.GetStringValue()
	case *protos.Value_Int32ArrayValues:
		return pv.GetInt32ArrayValues().GetInts()
	case *protos.Value_Float32ArrayValues:
		return pv.GetFloat32ArrayValues().GetFloats()
	case *protos.Value_StringArrayValues:
		return pv.GetStringArrayValues().GetStrings()
	case *protos.Value_StructValue:
		fields := pv.GetStructValue().GetFields()
		m := make(map[string]any, len(fields))
		for k, v := range fields {
			m[k] = FromProto(v)
		}
		return m
	case *protos.Value_StructArrayValues:
		list := pv.GetStructArrayValues().GetStructValues()
		arr := make([]map[string]any, len(list))
		for i, sv := range list {
			fields := sv.GetFields()
			m := make(map[string]any, len(fields))
			for k, v := range fields {
				m[k] = FromProto(v)
			}
			arr[i] = m
		}
		return arr
	case *protos.Value_StructVariantValue:
		sv := pv.GetStructVariantValue()
		return StructVariantValue{
			StructVariantType: sv.GetStructVariantType(),
			Value:             FromProto(sv.GetValue()),
		}
	case *protos.Value_StructVariantArrayValues:
		list := pv.GetStructVariantArrayValues().GetStructVariants()
		arr := make([]StructVariantValue, 0, len(list))
		for _, sv := range list {
			arr = append(arr, StructVariantValue{
				StructVariantType: sv.GetStructVariantType(),
				Value:             FromProto(sv.GetValue()),
			})
		}
		return arr
	case *protos.Value_DataPayload:
		dp := pv.GetDataPayload()

		// Convert encoding enum to string
		var encoding string
		switch dp.GetPayloadEncoding() {
		case protos.DataPayload_GZIP:
			encoding = "gzip"
		case protos.DataPayload_DEFLATE:
			encoding = "deflate"
		default:
			encoding = "uncompressed"
		}

		result := DataPayload{
			Metadata:        dp.GetMetadata(),
			Digest:          dp.GetDigest(),
			PayloadEncoding: encoding,
		}

		// Get either URL or Payload based on Kind
		switch kind := dp.GetKind().(type) {
		case *protos.DataPayload_Url:
			result.URL = kind.Url
		case *protos.DataPayload_Payload:
			result.Payload = kind.Payload
		}

		return result
	default:
		return nil
	}
}
