package main

import (
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

// Shared param makers keep the device descriptor (device_handler.go) and the
// GetParam handler (get_param_handler.go) in sync: both build a given OID's
// param the same way, so a single param never drifts between the two endpoints.
// Each maker returns a *catena.Param; the device handler calls .ToMap() while
// GetParam dereferences and returns it directly.

// makeCounterParam builds the slot 0 "counter" param (INT32, current value).
func makeCounterParam(counter *CounterState) *catena.Param {
	return catena.NewParamInt32(counter.GetValue()).
		WithName(catena.NewPolyglotText("en", "Counter").
			With("es", "Contador").
			With("fr", "Compteur").
			With("ja", "カウンター")).
		WithMinimalSet(true)
}

// makeRunningParam builds the slot 0 "running" param (INT32, read-only,
// INT32_CHOICE constraint describing the counting state).
func makeRunningParam(counter *CounterState) *catena.Param {
	return catena.NewParamInt32(counter.RunningInt32()).
		WithName(catena.NewPolyglotText("en", "Counter Running Status")).
		WithReadOnly(true).
		WithConstraint(catena.NewConstraintInt32Choice([]catena.Int32Choice{
			{Value: 0, Name: catena.NewPolyglotText("en", "Not Counting")},
			{Value: 1, Name: catena.NewPolyglotText("en", "Counting")},
		})).
		WithMinimalSet(true)
}
