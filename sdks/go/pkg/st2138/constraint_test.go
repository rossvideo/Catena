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

package st2138

import (
	"errors"
	"testing"

	"google.golang.org/protobuf/proto"

	"github.com/rossvideo/catena/sdks/go/pkg/protos"
)

func TestNewConstraintInt32Choice(t *testing.T) {
	c, err := NewConstraintInt32Choice([]Int32Choice{
		{Value: 1, Name: PolyglotText{"en": "one"}},
		{Value: 2, Name: PolyglotText{"en": "two"}},
	}).Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if c.Proto.GetType() != protos.Constraint_INT_CHOICE {
		t.Errorf("expected INT_CHOICE, got %v", c.Proto.GetType())
	}
	choices := c.Proto.GetInt32Choice().GetChoices()
	if len(choices) != 2 {
		t.Fatalf("expected 2 choices, got %d", len(choices))
	}
	if choices[0].GetValue() != 1 {
		t.Errorf("expected first choice value 1, got %d", choices[0].GetValue())
	}
	if choices[0].GetName().GetDisplayStrings()["en"] != "one" {
		t.Errorf("expected first choice name 'one', got %q", choices[0].GetName().GetDisplayStrings()["en"])
	}
	if choices[1].GetValue() != 2 {
		t.Errorf("expected second choice value 2, got %d", choices[1].GetValue())
	}
	if choices[1].GetName().GetDisplayStrings()["en"] != "two" {
		t.Errorf("expected second choice name 'two', got %q", choices[1].GetName().GetDisplayStrings()["en"])
	}
}

func TestNewConstraintInt32Choice_Empty(t *testing.T) {
	c, err := NewConstraintInt32Choice(nil).Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if c.Proto.GetType() != protos.Constraint_INT_CHOICE {
		t.Errorf("expected INT_CHOICE, got %v", c.Proto.GetType())
	}
	if len(c.Proto.GetInt32Choice().GetChoices()) != 0 {
		t.Error("expected empty choices")
	}
}

func TestNewConstraintInt32Range(t *testing.T) {
	c, err := NewConstraintInt32Range(0, 100, 5).Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if c.Proto.GetType() != protos.Constraint_INT_RANGE {
		t.Errorf("expected INT_RANGE, got %v", c.Proto.GetType())
	}
	r := c.Proto.GetInt32Range()
	if r.GetMinValue() != 0 {
		t.Errorf("expected min 0, got %d", r.GetMinValue())
	}
	if r.GetMaxValue() != 100 {
		t.Errorf("expected max 100, got %d", r.GetMaxValue())
	}
	if r.GetStep() != 5 {
		t.Errorf("expected step 5, got %d", r.GetStep())
	}
	if r.GetDisplayMin() != 0 || r.GetDisplayMax() != 0 {
		t.Error("expected display min/max to default to 0")
	}
}

func TestNewConstraintInt32RangeWithDisplay(t *testing.T) {
	c, err := NewConstraintInt32RangeWithDisplay(0, 100, 1, 10, 90).Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	r := c.Proto.GetInt32Range()
	if r.GetDisplayMin() != 10 {
		t.Errorf("expected display min 10, got %d", r.GetDisplayMin())
	}
	if r.GetDisplayMax() != 90 {
		t.Errorf("expected display max 90, got %d", r.GetDisplayMax())
	}
}

func TestNewConstraintFloatRange(t *testing.T) {
	c, err := NewConstraintFloatRange(0, 1, 0.01).Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if c.Proto.GetType() != protos.Constraint_FLOAT_RANGE {
		t.Errorf("expected FLOAT_RANGE, got %v", c.Proto.GetType())
	}
	r := c.Proto.GetFloatRange()
	if r.GetMinValue() != 0 {
		t.Errorf("expected min 0, got %f", r.GetMinValue())
	}
	if r.GetMaxValue() != 1 {
		t.Errorf("expected max 1, got %f", r.GetMaxValue())
	}
	if r.GetStep() != 0.01 {
		t.Errorf("expected step 0.01, got %f", r.GetStep())
	}
}

func TestNewConstraintFloatRangeWithDisplay(t *testing.T) {
	c, err := NewConstraintFloatRangeWithDisplay(0, 100, 0.5, 10, 90).Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	r := c.Proto.GetFloatRange()
	if r.GetDisplayMin() != 10 {
		t.Errorf("expected display min 10, got %f", r.GetDisplayMin())
	}
	if r.GetDisplayMax() != 90 {
		t.Errorf("expected display max 90, got %f", r.GetDisplayMax())
	}
}

func TestNewConstraintStringChoice(t *testing.T) {
	c, err := NewConstraintStringChoice(true, "a", "b", "c").Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if c.Proto.GetType() != protos.Constraint_STRING_CHOICE {
		t.Errorf("expected STRING_CHOICE, got %v", c.Proto.GetType())
	}
	sc := c.Proto.GetStringChoice()
	if !sc.GetStrict() {
		t.Error("expected strict=true")
	}
	if len(sc.GetChoices()) != 3 {
		t.Fatalf("expected 3 choices, got %d", len(sc.GetChoices()))
	}
	if sc.GetChoices()[0] != "a" || sc.GetChoices()[1] != "b" || sc.GetChoices()[2] != "c" {
		t.Errorf("unexpected choices: %v", sc.GetChoices())
	}
}

func TestNewConstraintStringChoice_NotStrict(t *testing.T) {
	c, err := NewConstraintStringChoice(false, "x").Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if c.Proto.GetStringChoice().GetStrict() {
		t.Error("expected strict=false")
	}
}

func TestNewConstraintStringStringChoice(t *testing.T) {
	c, err := NewConstraintStringStringChoice(true, []StringStringChoice{
		{Value: "r", Name: PolyglotText{"en": "Red"}},
		{Value: "g", Name: PolyglotText{"en": "Green"}},
	}).Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if c.Proto.GetType() != protos.Constraint_STRING_STRING_CHOICE {
		t.Errorf("expected STRING_STRING_CHOICE, got %v", c.Proto.GetType())
	}
	ssc := c.Proto.GetStringStringChoice()
	if !ssc.GetStrict() {
		t.Error("expected strict=true")
	}
	choices := ssc.GetChoices()
	if len(choices) != 2 {
		t.Fatalf("expected 2 choices, got %d", len(choices))
	}
	if choices[0].GetValue() != "r" {
		t.Errorf("expected first choice value 'r', got %q", choices[0].GetValue())
	}
	if choices[0].GetName().GetDisplayStrings()["en"] != "Red" {
		t.Errorf("expected first choice name 'Red', got %q", choices[0].GetName().GetDisplayStrings()["en"])
	}
}

func TestNewConstraintAlarmTable(t *testing.T) {
	c, err := NewConstraintAlarmTable([]AlarmEntry{
		{BitValue: 0, Severity: AlarmSeverityInfo, Description: PolyglotText{"en": "All clear"}},
		{BitValue: 1, Severity: AlarmSeverityWarning, Description: PolyglotText{"en": "Caution"}},
		{BitValue: 2, Severity: AlarmSeveritySevere},
	}).Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if c.Proto.GetType() != protos.Constraint_ALARM_TABLE {
		t.Errorf("expected ALARM_TABLE, got %v", c.Proto.GetType())
	}
	alarms := c.Proto.GetAlarmTable().GetAlarms()
	if len(alarms) != 3 {
		t.Fatalf("expected 3 alarms, got %d", len(alarms))
	}
	if alarms[0].GetBitValue() != 0 {
		t.Errorf("expected bit_value 0, got %d", alarms[0].GetBitValue())
	}
	if alarms[0].GetSeverity() != protos.Alarm_INFO {
		t.Errorf("expected INFO severity, got %v", alarms[0].GetSeverity())
	}
	if alarms[0].GetDescription().GetDisplayStrings()["en"] != "All clear" {
		t.Errorf("expected description 'All clear', got %q", alarms[0].GetDescription().GetDisplayStrings()["en"])
	}
	if alarms[1].GetSeverity() != protos.Alarm_WARNING {
		t.Errorf("expected WARNING severity, got %v", alarms[1].GetSeverity())
	}
	if alarms[2].GetDescription() != nil {
		t.Error("expected nil description for alarm with no Description set")
	}
}

func TestNewConstraintAlarmTable_Empty(t *testing.T) {
	c, err := NewConstraintAlarmTable(nil).Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if len(c.Proto.GetAlarmTable().GetAlarms()) != 0 {
		t.Error("expected empty alarms")
	}
}

func TestNewConstraintRefOid(t *testing.T) {
	c, err := NewConstraintRefOid("shared.my_constraint").Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if c.Proto.GetType() != protos.Constraint_UNDEFINED {
		t.Errorf("expected UNDEFINED type for ref_oid, got %v", c.Proto.GetType())
	}
	if c.Proto.GetRefOid() != "shared.my_constraint" {
		t.Errorf("expected 'shared.my_constraint', got %q", c.Proto.GetRefOid())
	}
}

func TestNewConstraintInt32Choice_MultiLanguage(t *testing.T) {
	c, err := NewConstraintInt32Choice([]Int32Choice{
		{Value: 1, Name: PolyglotText{"en": "one", "fr": "un"}},
	}).Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	name := c.Proto.GetInt32Choice().GetChoices()[0].GetName().GetDisplayStrings()
	if name["en"] != "one" {
		t.Errorf("expected 'one', got %q", name["en"])
	}
	if name["fr"] != "un" {
		t.Errorf("expected 'un', got %q", name["fr"])
	}
}

func TestConstraintBuilder_BuildsValidConstraint(t *testing.T) {
	c, err := NewConstraintInt32Choice([]Int32Choice{
		{Value: 1, Name: PolyglotText{"en": "one"}},
	}).Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if c.Proto == nil {
		t.Fatal("expected non-nil proto")
	}
	if c.Proto.GetType() != protos.Constraint_INT_CHOICE {
		t.Errorf("expected INT_CHOICE, got %v", c.Proto.GetType())
	}
	if len(c.Proto.GetInt32Choice().GetChoices()) != 1 {
		t.Errorf("expected 1 choice, got %d", len(c.Proto.GetInt32Choice().GetChoices()))
	}
}

func TestConstraintBuilder_BuildSurfacesErrors(t *testing.T) {
	cb := NewConstraintInt32Choice([]Int32Choice{
		{Value: 1, Name: PolyglotText{"en": "one"}},
	})
	cb.err = errors.New("test error")
	if _, err := cb.Build(); err == nil {
		t.Fatal("expected error for builder with error")
	}
}

func TestConstraintBuilder_Clone(t *testing.T) {
	cb := NewConstraintInt32Choice([]Int32Choice{
		{Value: 1, Name: PolyglotText{"en": "one"}},
	})
	clone := cb.Clone()
	if !proto.Equal(cb.proto, clone.proto) {
		t.Fatal("expected clone to be equal to original")
	}
	if clone == cb {
		t.Fatal("expected clone to be a different object")
	}
	if clone.proto == cb.proto {
		t.Fatal("expected clone to have a different proto")
	}
}

func TestConstraintBuilder_CloneInheritsError(t *testing.T) {

	cb := NewConstraintInt32Choice([]Int32Choice{
		{Value: 1, Name: PolyglotText{"en": "one"}},
	})
	cb.err = errors.New("test error")
	clone := cb.Clone()
	if clone.err == nil || clone.err.Error() != "test error" {
		t.Fatal("expected clone to inherit error")
	}
}

func TestConstraint_AlreadyBuilt(t *testing.T) {
	cb := NewConstraintInt32Choice([]Int32Choice{
		{Value: 1, Name: PolyglotText{"en": "one"}},
	})
	c1, err := cb.Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if c1.Proto.GetType() != protos.Constraint_INT_CHOICE {
		t.Errorf("expected INT_CHOICE, got %v", c1.Proto.GetType())
	}
	c2, err := cb.Build()
	if err == nil || !errors.Is(err, ErrAlreadyBuilt) {
		t.Fatal("expected error for builder after Build(), got", err)
	}
	if c2 != (Constraint{}) {
		t.Fatal("expected empty constraint for building already built constraint, got", c2)
	}
}

func TestConstraintBuilder_CloneAlreadyBuilt(t *testing.T) {
	cb := NewConstraintInt32Choice([]Int32Choice{
		{Value: 1, Name: PolyglotText{"en": "one"}},
	})
	c1, err := cb.Build()
	if err != nil {
		t.Fatalf("expected no error, got %v", err)
	}
	if c1.Proto.GetType() != protos.Constraint_INT_CHOICE {
		t.Errorf("expected INT_CHOICE, got %v", c1.Proto.GetType())
	}
	clone := cb.Clone()
	c2, err := clone.Build()
	if err == nil || !errors.Is(err, ErrAlreadyBuilt) {
		t.Fatal("expected error for clone after Build(), got", err)
	}
	if c2 != (Constraint{}) {
		t.Fatal("expected empty constraint for clone after Build(), got", c2)
	}
}

func TestAlarmSeverityConstants(t *testing.T) {
	tests := []struct {
		got  AlarmSeverity
		want protos.Alarm_Severity
	}{
		{AlarmSeverityInfo, protos.Alarm_INFO},
		{AlarmSeverityWarning, protos.Alarm_WARNING},
		{AlarmSeveritySevere, protos.Alarm_SEVERE},
		{AlarmSeverityUnknown, protos.Alarm_UNKNOWN},
		{AlarmSeverityRefused, protos.Alarm_REFUSED},
	}
	for _, tt := range tests {
		if tt.got != tt.want {
			t.Errorf("severity mismatch: got %v, want %v", tt.got, tt.want)
		}
	}
}
