package catena

import (
	"fmt"

	"google.golang.org/protobuf/encoding/protojson"

	"github.com/rossvideo/catena/build/go/protos"
)

// CatenaAsset wraps protos.ExternalObjectPayload for asset handling
type CatenaAsset struct {
	asset *protos.ExternalObjectPayload
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

// ToCatenaAsset converts DataPayload to CatenaAsset by building the proto directly
func ToCatenaAsset(dp DataPayload, cachable bool) (CatenaAsset, error) {
	// Build the proto DataPayload directly
	protoPayload := &protos.DataPayload{
		Metadata:        dp.Metadata,
		Digest:          dp.Digest,
		PayloadEncoding: protos.DataPayload_PayloadEncoding(dp.PayloadEncoding),
	}

	// Handle oneof: either url or payload (not both)
	if dp.Url != "" && len(dp.Payload) == 0 {
		protoPayload.Kind = &protos.DataPayload_Url{Url: dp.Url}
	} else if len(dp.Payload) > 0 && dp.Url == "" {
		protoPayload.Kind = &protos.DataPayload_Payload{Payload: dp.Payload}
	} else {
		return CatenaAsset{asset: nil}, fmt.Errorf("either payload or url must be provided in DataPayload, but not both")
	}

	// Build the ExternalObjectPayload
	asset := &protos.ExternalObjectPayload{
		Cachable: cachable,
		Payload:  protoPayload,
	}

	return CatenaAsset{asset: asset}, nil
}

// GetProtoAsset returns the underlying protos.ExternalObjectPayload
func (ca CatenaAsset) GetProtoAsset() *protos.ExternalObjectPayload {
	return ca.asset
}

// ToJSON converts CatenaAsset to JSON bytes using protojson
func (ca CatenaAsset) ToJSON() ([]byte, error) {
	if ca.asset == nil {
		return nil, nil
	}

	return (protojson.MarshalOptions{
		UseProtoNames:   true,
		EmitUnpopulated: false,
	}).Marshal(ca.asset)
}
