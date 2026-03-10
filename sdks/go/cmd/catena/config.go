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

package main

import (
	"fmt"
	"os"
	"path/filepath"

	"gopkg.in/yaml.v3"
)

// CatenaConfig represents the top-level YAML configuration
type CatenaConfig struct {
	SlotMap  map[int]SlotEntry `yaml:"slot_map"`
	LogLevel string            `yaml:"log_level"`
	Port     int               `yaml:"port"`
}

// SlotEntry represents a plugin configuration for a slot.
// It can be either a simple string (path) or a full configuration map.
type SlotEntry struct {
	Path  string      `yaml:"path"`
	Args  []Arg       `yaml:"args"`
	Trust *TrustInfo  `yaml:"trust"`
}

// Arg represents a name-value argument pair
type Arg struct {
	Name  string `yaml:"name"`
	Value string `yaml:"value"`
}

// TrustInfo contains information about why a module should be trusted
type TrustInfo struct {
	Anchor    string `yaml:"anchor"`
	Signature string `yaml:"signature"`
}

// UnmarshalYAML implements custom unmarshaling to handle both string and map formats
func (s *SlotEntry) UnmarshalYAML(node *yaml.Node) error {
	// Try to unmarshal as a simple string first
	if node.Kind == yaml.ScalarNode {
		s.Path = node.Value
		return nil
	}

	// Otherwise unmarshal as a map
	type slotEntryAlias SlotEntry
	var entry slotEntryAlias
	if err := node.Decode(&entry); err != nil {
		return err
	}
	*s = SlotEntry(entry)
	return nil
}

// LoadConfig loads and parses a YAML configuration file
func LoadConfig(path string) (*CatenaConfig, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("failed to read config file: %w", err)
	}

	var config CatenaConfig
	if err := yaml.Unmarshal(data, &config); err != nil {
		return nil, fmt.Errorf("failed to parse config file: %w", err)
	}

	// Resolve relative paths relative to the config file's directory
	configDir := filepath.Dir(path)
	for slot, entry := range config.SlotMap {
		if entry.Path != "" && !filepath.IsAbs(entry.Path) {
			entry.Path = filepath.Join(configDir, entry.Path)
			config.SlotMap[slot] = entry
		}
	}

	return &config, nil
}

// ToArgMap converts Args slice to a map for easier lookup
func (s *SlotEntry) ToArgMap() map[string]string {
	m := make(map[string]string)
	for _, arg := range s.Args {
		m[arg.Name] = arg.Value
	}
	return m
}
