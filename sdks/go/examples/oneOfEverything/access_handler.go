package main

import (
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

func registerAccessHandler(srv catena.Server) {
	// AccessHandler is the coarse authorization gate for SDK endpoints. It runs
	// after the transport has built HandlerContext, so adopters can check scopes,
	// identity, or endpoint type before any endpoint-specific handler runs. Use
	// this for cross-cutting policy; use individual handlers for per-param rules.
	srv.RegisterAccessHandler(func(endpointType catena.EndpointType, ctx catena.HandlerContext) bool {
		logger.Info("Access request", "endpointType", endpointType)

		// Shows how to restrict getPopulatedSlots to only op and adm read scopes.
		//if endpointType == catena.EndpointGetSlots {
		//	return ctx.HasReadScope(catena.ScopeOp) || ctx.HasReadScope(catena.ScopeAdm)
		//}
		return true
	})
}
