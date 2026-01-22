package catena

import (
	"fmt"
	"github.com/rossvideo/catena/build/go/protos"
)

// DataPayload is a Go-friendly representation of asset data.
// Use this in business logic to build assets before converting to CatenaAsset.
type DataPayload struct {
	// Either Data or URL must be set, not both
	Data []byte // Embedded binary data
	URL  string // External URL reference

	// Metadata
	ContentType        string            // MIME type
	ContentDisposition string            // How to display/download the file
	CustomHeaders      map[string]string // Additional HTTP headers
	Digest             []byte            // Checksum/hash of the data

	// Options
	Cachable        bool   // Whether the asset can be cached
	PayloadEncoding string // "gzip", "deflate", or "" (uncompressed)
}

// ToCatenaAsset converts a DataPayload to CatenaAsset
func (dp DataPayload) ToCatenaAsset() CatenaAsset {
	// Build metadata map
	metadata := make(map[string]string)
	if dp.ContentType != "" {
		metadata["content-type"] = dp.ContentType
	}
	if dp.ContentDisposition != "" {
		metadata["content-disposition"] = dp.ContentDisposition
	}
	for k, v := range dp.CustomHeaders {
		metadata[k] = v
	}

	// Determine payload encoding
	var encoding protos.DataPayload_PayloadEncoding
	switch dp.PayloadEncoding {
	case "gzip":
		encoding = protos.DataPayload_GZIP
	case "deflate":
		encoding = protos.DataPayload_DEFLATE
	default:
		encoding = protos.DataPayload_UNCOMPRESSED
	}

	// Create the protobuf DataPayload with appropriate Kind
	protoPayload := &protos.DataPayload{
		Metadata:        metadata,
		Digest:          dp.Digest,
		PayloadEncoding: encoding,
	}

	if dp.URL != "" {
		// URL-based asset
		protoPayload.Kind = &protos.DataPayload_Url{Url: dp.URL}
	} else {
		// Embedded data asset
		protoPayload.Kind = &protos.DataPayload_Payload{Payload: dp.Data}
	}

	return CatenaAsset{
		ExternalObject: &protos.ExternalObjectPayload{
			Cachable: dp.Cachable,
			Payload:  protoPayload,
		},
	}
}

// CatenaAsset wraps protos.ExternalObjectPayload for asset handling
type CatenaAsset struct {
	ExternalObject *protos.ExternalObjectPayload
}

// NewCatenaAsset creates a CatenaAsset from binary data and content type
func NewCatenaAsset(data []byte, contentType string) CatenaAsset {
	return DataPayload{
		Data:        data,
		ContentType: contentType,
		Cachable:    true,
	}.ToCatenaAsset()
}

// NewCatenaAssetFromURL creates a CatenaAsset that references an external URL
func NewCatenaAssetFromURL(url string, contentType string) CatenaAsset {
	return DataPayload{
		URL:         url,
		ContentType: contentType,
		Cachable:    true,
	}.ToCatenaAsset()
}

// NewCatenaAssetFromProto creates a CatenaAsset from a protos.ExternalObjectPayload
func NewCatenaAssetFromProto(eop *protos.ExternalObjectPayload) CatenaAsset {
	return CatenaAsset{ExternalObject: eop}
}

// NewCatenaAssetFromDataPayload creates a CatenaAsset from a protos.DataPayload
func NewCatenaAssetFromDataPayload(dp *protos.DataPayload) CatenaAsset {
	return CatenaAsset{
		ExternalObject: &protos.ExternalObjectPayload{
			Cachable: true,
			Payload:  dp,
		},
	}
}

// isValid checks if the asset has valid structure
func (ca CatenaAsset) isValid() bool {
	return ca.ExternalObject != nil && ca.ExternalObject.Payload != nil
}

// WithFilename sets the Content-Disposition header for download
func (ca CatenaAsset) WithFilename(filename string) CatenaAsset {
	if !ca.isValid() {
		return ca
	}
	if ca.ExternalObject.Payload.Metadata == nil {
		ca.ExternalObject.Payload.Metadata = make(map[string]string)
	}
	ca.ExternalObject.Payload.Metadata["content-disposition"] = fmt.Sprintf("attachment; filename=\"%s\"", filename)
	return ca
}

// WithInlineFilename sets the Content-Disposition header for inline display
func (ca CatenaAsset) WithInlineFilename(filename string) CatenaAsset {
	if !ca.isValid() {
		return ca
	}
	if ca.ExternalObject.Payload.Metadata == nil {
		ca.ExternalObject.Payload.Metadata = make(map[string]string)
	}
	ca.ExternalObject.Payload.Metadata["content-disposition"] = fmt.Sprintf("inline; filename=\"%s\"", filename)
	return ca
}

// WithCustomHeader adds a custom HTTP header
func (ca CatenaAsset) WithCustomHeader(key, value string) CatenaAsset {
	if !ca.isValid() {
		return ca
	}
	if ca.ExternalObject.Payload.Metadata == nil {
		ca.ExternalObject.Payload.Metadata = make(map[string]string)
	}
	ca.ExternalObject.Payload.Metadata[key] = value
	return ca
}

// WithCompression sets the payload encoding
func (ca CatenaAsset) WithCompression(encoding string) CatenaAsset {
	if !ca.isValid() {
		return ca
	}
	switch encoding {
	case "gzip":
		ca.ExternalObject.Payload.PayloadEncoding = protos.DataPayload_GZIP
	case "deflate":
		ca.ExternalObject.Payload.PayloadEncoding = protos.DataPayload_DEFLATE
	default:
		ca.ExternalObject.Payload.PayloadEncoding = protos.DataPayload_UNCOMPRESSED
	}
	return ca
}

// WithDigest sets the digest/checksum for the payload
func (ca CatenaAsset) WithDigest(digest []byte) CatenaAsset {
	if !ca.isValid() {
		return ca
	}
	ca.ExternalObject.Payload.Digest = digest
	return ca
}

// WithMetadata adds metadata to the payload
func (ca CatenaAsset) WithMetadata(key, value string) CatenaAsset {
	if !ca.isValid() {
		return ca
	}
	if ca.ExternalObject.Payload.Metadata == nil {
		ca.ExternalObject.Payload.Metadata = make(map[string]string)
	}
	ca.ExternalObject.Payload.Metadata[key] = value
	return ca
}

// SetCachable sets whether the asset should be cached
func (ca CatenaAsset) SetCachable(cachable bool) CatenaAsset {
	if ca.ExternalObject == nil {
		return ca
	}
	ca.ExternalObject.Cachable = cachable
	return ca
}

// IsURL returns true if this asset is a URL reference
func (ca CatenaAsset) IsURL() bool {
	if !ca.isValid() {
		return false
	}
	_, ok := ca.ExternalObject.Payload.Kind.(*protos.DataPayload_Url)
	return ok
}

// GetData returns the embedded payload data (nil if URL-based)
func (ca CatenaAsset) GetData() []byte {
	if !ca.isValid() {
		return nil
	}
	if payload, ok := ca.ExternalObject.Payload.Kind.(*protos.DataPayload_Payload); ok {
		return payload.Payload
	}
	return nil
}

// GetURL returns the URL (empty string if embedded data)
func (ca CatenaAsset) GetURL() string {
	if !ca.isValid() {
		return ""
	}
	if url, ok := ca.ExternalObject.Payload.Kind.(*protos.DataPayload_Url); ok {
		return url.Url
	}
	return ""
}

// ContentLength returns the size of the payload (0 for URL-based assets)
func (ca CatenaAsset) ContentLength() int64 {
	data := ca.GetData()
	if data != nil {
		return int64(len(data))
	}
	return 0
}

// GetContentType returns the content-type from metadata
func (ca CatenaAsset) GetContentType() string {
	if !ca.isValid() || ca.ExternalObject.Payload.Metadata == nil {
		return ""
	}
	return ca.ExternalObject.Payload.Metadata["content-type"]
}

// GetContentDisposition returns the content-disposition from metadata
func (ca CatenaAsset) GetContentDisposition() string {
	if !ca.isValid() || ca.ExternalObject.Payload.Metadata == nil {
		return ""
	}
	return ca.ExternalObject.Payload.Metadata["content-disposition"]
}

// GetMetadata returns a specific metadata value
func (ca CatenaAsset) GetMetadata(key string) string {
	if !ca.isValid() || ca.ExternalObject.Payload.Metadata == nil {
		return ""
	}
	return ca.ExternalObject.Payload.Metadata[key]
}

// GetAllMetadata returns all metadata
func (ca CatenaAsset) GetAllMetadata() map[string]string {
	if !ca.isValid() {
		return nil
	}
	return ca.ExternalObject.Payload.Metadata
}

// IsCachable returns whether the asset is cachable
func (ca CatenaAsset) IsCachable() bool {
	if ca.ExternalObject == nil {
		return false
	}
	return ca.ExternalObject.Cachable
}
