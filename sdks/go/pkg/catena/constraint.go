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

/**
 * @brief Constraint builders for the Catena SDK.
 * @file constraint.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @date 2026-05-29
 */

package catena

import (
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

// Constraint wraps a protos.Constraint and exposes factory functions for
// every constraint kind defined in the protocol.
type Constraint struct {
	Proto *protos.Constraint
}

// --- Input types for constraint builders ---

// Int32Choice pairs an int32 value with a human-readable display name.
type Int32Choice struct {
	Value int32
	Name  PolyglotText
}

// StringStringChoice pairs a machine-use string value with a human-readable
// display name.
type StringStringChoice struct {
	Value string
	Name  PolyglotText
}

// AlarmSeverity is the severity level of an alarm entry.
type AlarmSeverity = protos.Alarm_Severity

const (
	AlarmSeverityInfo    AlarmSeverity = protos.Alarm_INFO
	AlarmSeverityWarning AlarmSeverity = protos.Alarm_WARNING
	AlarmSeveritySevere  AlarmSeverity = protos.Alarm_SEVERE
	AlarmSeverityUnknown AlarmSeverity = protos.Alarm_UNKNOWN
	AlarmSeverityRefused AlarmSeverity = protos.Alarm_REFUSED
)

// AlarmEntry describes a single alarm in an alarm table constraint.
type AlarmEntry struct {
	BitValue    int32
	Severity    AlarmSeverity
	Description PolyglotText
}

// --- Factory functions ---

// NewConstraintInt32Choice creates an INT_CHOICE constraint from a list of
// int32 value/name pairs.
func NewConstraintInt32Choice(choices []Int32Choice) *Constraint {
	pbChoices := make([]*protos.Int32ChoiceConstraint_IntChoice, len(choices))
	for i, c := range choices {
		pbChoices[i] = &protos.Int32ChoiceConstraint_IntChoice{
			Value: c.Value,
			Name:  &protos.PolyglotText{DisplayStrings: c.Name},
		}
	}
	return &Constraint{
		Proto: &protos.Constraint{
			Type: protos.Constraint_INT_CHOICE,
			Kind: &protos.Constraint_Int32Choice{
				Int32Choice: &protos.Int32ChoiceConstraint{Choices: pbChoices},
			},
		},
	}
}

// NewConstraintInt32Range creates an INT_RANGE constraint with the given
// min, max, and step. Display min/max default to 0 (use
// NewConstraintInt32RangeWithDisplay to set them explicitly).
func NewConstraintInt32Range(min, max, step int32) *Constraint {
	return &Constraint{
		Proto: &protos.Constraint{
			Type: protos.Constraint_INT_RANGE,
			Kind: &protos.Constraint_Int32Range{
				Int32Range: &protos.Int32RangeConstraint{
					MinValue: min,
					MaxValue: max,
					Step:     step,
				},
			},
		},
	}
}

// NewConstraintInt32RangeWithDisplay creates an INT_RANGE constraint with
// explicit display bounds.
func NewConstraintInt32RangeWithDisplay(min, max, step, displayMin, displayMax int32) *Constraint {
	return &Constraint{
		Proto: &protos.Constraint{
			Type: protos.Constraint_INT_RANGE,
			Kind: &protos.Constraint_Int32Range{
				Int32Range: &protos.Int32RangeConstraint{
					MinValue:   min,
					MaxValue:   max,
					Step:       step,
					DisplayMin: displayMin,
					DisplayMax: displayMax,
				},
			},
		},
	}
}

// NewConstraintFloatRange creates a FLOAT_RANGE constraint with the given
// min, max, and step. Display min/max default to 0 (use
// NewConstraintFloatRangeWithDisplay to set them explicitly).
func NewConstraintFloatRange(min, max, step float32) *Constraint {
	return &Constraint{
		Proto: &protos.Constraint{
			Type: protos.Constraint_FLOAT_RANGE,
			Kind: &protos.Constraint_FloatRange{
				FloatRange: &protos.FloatRangeConstraint{
					MinValue: min,
					MaxValue: max,
					Step:     step,
				},
			},
		},
	}
}

// NewConstraintFloatRangeWithDisplay creates a FLOAT_RANGE constraint with
// explicit display bounds.
func NewConstraintFloatRangeWithDisplay(min, max, step, displayMin, displayMax float32) *Constraint {
	return &Constraint{
		Proto: &protos.Constraint{
			Type: protos.Constraint_FLOAT_RANGE,
			Kind: &protos.Constraint_FloatRange{
				FloatRange: &protos.FloatRangeConstraint{
					MinValue:   min,
					MaxValue:   max,
					Step:       step,
					DisplayMin: displayMin,
					DisplayMax: displayMax,
				},
			},
		},
	}
}

// NewConstraintStringChoice creates a STRING_CHOICE constraint. When strict
// is true the value must be one of the provided choices.
func NewConstraintStringChoice(strict bool, choices ...string) *Constraint {
	return &Constraint{
		Proto: &protos.Constraint{
			Type: protos.Constraint_STRING_CHOICE,
			Kind: &protos.Constraint_StringChoice{
				StringChoice: &protos.StringChoiceConstraint{
					Choices: choices,
					Strict:  strict,
				},
			},
		},
	}
}

// NewConstraintStringStringChoice creates a STRING_STRING_CHOICE constraint
// from a list of machine-value / display-name pairs. When strict is true the
// value must be one of the provided choices.
func NewConstraintStringStringChoice(strict bool, choices []StringStringChoice) *Constraint {
	pbChoices := make([]*protos.StringStringChoiceConstraint_StringStringChoice, len(choices))
	for i, c := range choices {
		pbChoices[i] = &protos.StringStringChoiceConstraint_StringStringChoice{
			Value: c.Value,
			Name:  &protos.PolyglotText{DisplayStrings: c.Name},
		}
	}
	return &Constraint{
		Proto: &protos.Constraint{
			Type: protos.Constraint_STRING_STRING_CHOICE,
			Kind: &protos.Constraint_StringStringChoice{
				StringStringChoice: &protos.StringStringChoiceConstraint{
					Choices: pbChoices,
					Strict:  strict,
				},
			},
		},
	}
}

// NewConstraintAlarmTable creates an ALARM_TABLE constraint from alarm entries.
func NewConstraintAlarmTable(alarms []AlarmEntry) *Constraint {
	pbAlarms := make([]*protos.Alarm, len(alarms))
	for i, a := range alarms {
		pbAlarms[i] = &protos.Alarm{
			BitValue: a.BitValue,
			Severity: a.Severity,
		}
		if a.Description != nil {
			pbAlarms[i].Description = &protos.PolyglotText{DisplayStrings: a.Description}
		}
	}
	return &Constraint{
		Proto: &protos.Constraint{
			Type: protos.Constraint_ALARM_TABLE,
			Kind: &protos.Constraint_AlarmTable{
				AlarmTable: &protos.AlarmTableConstraint{Alarms: pbAlarms},
			},
		},
	}
}

// NewConstraintRefOid creates a constraint that references a shared
// constraint defined in device.constraints by its OID.
func NewConstraintRefOid(oid string) *Constraint {
	return &Constraint{
		Proto: &protos.Constraint{
			Type: protos.Constraint_UNDEFINED,
			Kind: &protos.Constraint_RefOid{RefOid: oid},
		},
	}
}
