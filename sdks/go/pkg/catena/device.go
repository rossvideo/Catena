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
