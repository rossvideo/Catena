package main

import (
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
)

// Shared param makers are the single source of truth for slot 0's params. The
// device descriptor (device_handler.go) passes each maker to WithParam; GetParam
// (get_param_handler.go) rebuilds that same device definition and reads the
// requested OID back out of it. Because both endpoints trace back to these
// makers, a param can never drift between GetDevice and GetParam.

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
