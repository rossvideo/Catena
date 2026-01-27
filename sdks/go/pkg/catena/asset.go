package catena

import (
	"encoding/json"
	"fmt"

	"google.golang.org/protobuf/encoding/protojson"

	"github.com/rossvideo/catena/build/go/protos"
)

// CatenaAsset wraps protos.ExternalObjectPayload for asset handling
type CatenaAsset struct {
	asset    *protos.ExternalObjectPayload
	jsonData []byte // Cached JSON representation
}

// DataPayload is a helper type for business logic to create assets
// It provides a convenient way to construct ExternalObjectPayload
type DataPayload struct {
	Data               []byte
	ContentType        string
	ContentDisposition string
	Cachable           bool
	CustomHeaders      map[string]string
	PayloadEncoding    int32 // 0=UNCOMPRESSED, 1=GZIP, 2=DEFLATE
}

// ToCatenaAsset converts DataPayload to CatenaAsset
func (dp DataPayload) ToCatenaAsset() CatenaAsset {
	// Build metadata map
	metadata := make(map[string]string)

	// Add standard headers
	if dp.ContentType != "" {
		metadata["content-type"] = dp.ContentType
	}
	if dp.ContentDisposition != "" {
		metadata["content-disposition"] = dp.ContentDisposition
	}

	// Add custom headers
	for key, value := range dp.CustomHeaders {
		metadata[key] = value
	}

	// Create the map structure for ExternalObjectPayload
	assetMap := map[string]any{
		"cachable": dp.Cachable,
		"payload": map[string]any{
			"payload":          dp.Data,
			"metadata":         metadata,
			"payload_encoding": dp.PayloadEncoding,
		},
	}

	// Convert to CatenaAsset
	catenaAsset, err := ToCatenaAsset(assetMap)
	if err != nil {
		// In case of error, return an empty CatenaAsset
		// The error will be logged by the caller
		return CatenaAsset{asset: nil, jsonData: nil}
	}

	return catenaAsset
}

// NewCatenaAsset creates a CatenaAsset from a protos.ExternalObjectPayload
func NewCatenaAsset(asset *protos.ExternalObjectPayload) CatenaAsset {
	return CatenaAsset{asset: asset}
}

// ToCatenaAsset converts a Go map/struct to CatenaAsset
// This allows developers to work with native Go types and convert to the protobuf format
func ToCatenaAsset(m map[string]any) (CatenaAsset, error) {
	asset, jsonData, err := toProtoAsset(m)
	if err != nil {
		return CatenaAsset{asset: nil, jsonData: nil}, fmt.Errorf("ToCatenaAsset: %w", err)
	}
	return CatenaAsset{asset: asset, jsonData: jsonData}, nil
}

// toProtoAsset converts native Go types to protos.ExternalObjectPayload
// For complex nested types, uses JSON marshaling to leverage protojson's
// automatic conversion and validation
// Returns the asset, cached JSON data, and any error
func toProtoAsset(m map[string]any) (*protos.ExternalObjectPayload, []byte, error) {
	// For complex types, use JSON marshaling to handle nested structures
	jsonData, err := json.Marshal(m)
	if err != nil {
		return nil, nil, fmt.Errorf("failed to marshal map to JSON: %w", err)
	}

	asset := &protos.ExternalObjectPayload{}
	if err := protojson.Unmarshal(jsonData, asset); err != nil {
		return nil, nil, fmt.Errorf("failed to unmarshal JSON to ExternalObjectPayload proto: %w", err)
	}

	return asset, jsonData, nil
}

// IsCachable returns whether the asset is cachable
func (ca CatenaAsset) IsCachable() bool {
	if ca.asset == nil {
		return false
	}
	return ca.asset.Cachable
}

// GetPayload returns the asset's data payload
func (ca CatenaAsset) GetPayload() *protos.DataPayload {
	if ca.asset == nil {
		return nil
	}
	return ca.asset.Payload
}

// GetProtoAsset returns the underlying protos.ExternalObjectPayload
func (ca CatenaAsset) GetProtoAsset() *protos.ExternalObjectPayload {
	return ca.asset
}

// ToJSON converts CatenaAsset to JSON bytes using protojson
// Uses cached JSON if available, otherwise generates new JSON
func (ca CatenaAsset) ToJSON() ([]byte, error) {
	if ca.asset == nil {
		return nil, nil
	}

	// Return cached JSON if available
	if ca.jsonData != nil {
		return ca.jsonData, nil
	}

	// Generate JSON from protobuf if not cached
	return (protojson.MarshalOptions{
		UseProtoNames:   true,
		EmitUnpopulated: false,
	}).Marshal(ca.asset)
}
