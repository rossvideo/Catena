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
// It mirrors protos.DataPayload structure for convenient construction
type DataPayload struct {
	Metadata        map[string]string // Information about the payload (e.g. content-type, filename)
	Digest          []byte            // SHA-256 digest of the payload
	PayloadEncoding int32             // 0=UNCOMPRESSED, 1=GZIP, 2=DEFLATE
	Payload         []byte            // Embedded binary data (use this OR Url, not both)
	Url             string            // URL to external resource (use this OR Payload, not both)
}

// ToCatenaAsset converts DataPayload to CatenaAsset
func (dp DataPayload) ToCatenaAsset(cachable bool) CatenaAsset {
	// Build the payload map for DataPayload
	payloadMap := map[string]any{
		"payload_encoding": dp.PayloadEncoding,
	}

	// Add metadata if present
	if len(dp.Metadata) > 0 {
		payloadMap["metadata"] = dp.Metadata
	}

	// Add digest if present
	if len(dp.Digest) > 0 {
		payloadMap["digest"] = dp.Digest
	}

	// Add either payload or url (oneof in proto)
	if dp.Url != "" {
		payloadMap["url"] = dp.Url
	} else if len(dp.Payload) > 0 {
		payloadMap["payload"] = dp.Payload
	}

	// Create the map structure for ExternalObjectPayload
	assetMap := map[string]any{
		"cachable": cachable,
		"payload":  payloadMap,
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
