package main

import "github.com/rossvideo/catena/sdks/go/pkg/catena"

func registerHeartbeatHandlers(srv catena.Server, state *ExampleState) {
	// Counter ticks often enough in slot 0 that a heartbeat isn't needed.

	// HeartbeatHandler lets a slot publish periodic activity for subscription
	// clients even when normal values are quiet. The SDK calls these handlers from
	// StartHeartbeat; handlers commonly BroadcastUpdate for a stable status param
	// so REST SSE and gRPC subscribers know the slot is still alive.
	srv.RegisterHeartbeatHandler(1, func(slot uint16) {
		// Example ticking on a slotOneParams key so GetValue can read it back.
		state.mu.RLock()
		resolution, _ := state.slotOneParams.Load("resolution")
		state.mu.RUnlock()
		srv.BroadcastUpdate(1, "resolution", resolution, catena.ScopeMon)
	})
	srv.RegisterHeartbeatHandler(2, func(slot uint16) {
		// Example ticking on a different param to show any can be used for heartbeats.
		state.mu.RLock()
		volume := state.volume
		state.mu.RUnlock()
		srv.BroadcastUpdate(2, "volume", volume, catena.ScopeMon)
	})
}
