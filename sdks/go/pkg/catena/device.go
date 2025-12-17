package catena

import (
	"context"
	"errors"
	"io"
	"os"
	"sync"
)

type Value struct {
	String string `json:"string,omitempty"`
	// Extend with numeric, boolean, etc.
}

type DeviceDescriptor struct {
	Id      string `json:"id"`
	Vendor  string `json:"vendor"`
	Product string `json:"product"`
	// plus menus, commands, params, etc.
}

// Device is what your business-logic implements.
// This is the "lite" device model from Catena’s perspective.
type Device interface {
	Describe(ctx context.Context) (*DeviceDescriptor, error)
	GetParam(ctx context.Context, path string) (Value, error)
	SetParam(ctx context.Context, path string, v Value) error
	GetAsset(ctx context.Context, id string) (io.ReadCloser, error)
}

// Simple in-memory implementation for an example device.
type simpleDevice struct {
	mu     sync.RWMutex
	status string
	// imagine a map of assetID -> path
	assetRoot string
}

func NewSimpleDevice(assetRoot string) Device {
	return &simpleDevice{
		status:    "OK",
		assetRoot: assetRoot,
	}
}

func (d *simpleDevice) Describe(ctx context.Context) (*DeviceDescriptor, error) {
	return &DeviceDescriptor{
		Id:      "catena-go-example",
		Vendor:  "Ross Video",
		Product: "Catena Go REST Example",
	}, nil
}

func (d *simpleDevice) GetParam(ctx context.Context, path string) (Value, error) {
	d.mu.RLock()
	defer d.mu.RUnlock()
	switch path {
	case "status":
		return Value{String: d.status}, nil
	default:
		return Value{}, errors.New("param not found")
	}
}

func (d *simpleDevice) SetParam(ctx context.Context, path string, v Value) error {
	d.mu.Lock()
	defer d.mu.Unlock()
	switch path {
	case "status":
		d.status = v.String
		return nil
	default:
		return errors.New("param not found")
	}
}

func (d *simpleDevice) GetAsset(ctx context.Context, id string) (io.ReadCloser, error) {
	// In real Catena, you'd respect the asset model; here we just map id -> file
	f, err := os.Open(d.assetRoot + "/" + id)
	if err != nil {
		return nil, err
	}
	return f, nil
}
