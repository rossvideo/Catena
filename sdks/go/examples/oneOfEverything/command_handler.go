package main

import (
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

func registerCommandHandler(srv catena.Server, counter *CounterState, broadcastRunning func(), counterScope string) {
	// Slot 0: switch per command FQOID and build command responses manually.
	// This handler does not reduce allowed scopes beyond the base level of
	// any command scope (mon, op, conf, or adm), however it broadcasts updates
	// to the counter value only to specified counterScope.
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any, ctx catena.HandlerContext) (catena.CommandResult, catena.StatusResult) {
		logger.Info("ExecuteCommand", "slot", slot, "command", commandFqoid)
		if !ctx.HasWriteScope(counterScope) {
			logger.Warning("Unauthorized command execution attempt", "slot", slot, "command", commandFqoid)
			return catena.CommandError(catena.StatusCodePermissionDenied, "Caller does not have required scope for this command")
		}

		switch commandFqoid {
		case "start":
			if counter.IsRunning() {
				logger.Info("Start command - already running")
			} else {
				counter.Start()
				logger.Info("Counter started", "value", counter.GetValue())
				broadcastRunning()
			}
			srv.BroadcastUpdate(0, "counter", counter.GetValue(), counterScope)
			val, _ := catena.ToValue(counter.GetValue())
			return catena.CommandReply(val)
		case "stop":
			if !counter.IsRunning() {
				logger.Info("Stop command - already stopped")
			} else {
				counter.Stop()
				logger.Info("Counter stopped", "value", counter.GetValue())
				broadcastRunning()
			}
			srv.BroadcastUpdate(0, "counter", counter.GetValue(), counterScope)
			val, _ := catena.ToValue(counter.GetValue())
			return catena.CommandReply(val)
		case "add10":
			counter.Add(10)
			logger.Info("Added 10 to counter", "value", counter.GetValue())
			srv.BroadcastUpdate(0, "counter", counter.GetValue(), counterScope)
			val, _ := catena.ToValue(counter.GetValue())
			return catena.CommandReply(val)
		case "reset":
			counter.Reset()
			logger.Info("Counter reset", "value", counter.GetValue())
			srv.BroadcastUpdate(0, "counter", counter.GetValue(), counterScope)
			val, _ := catena.ToValue(counter.GetValue())
			return catena.CommandReply(val)
		default:
			logger.Warning("Command not found", "slot", slot, "command", commandFqoid)
			return catena.CommandError(catena.StatusCodeNotFound, "Command not found: "+commandFqoid)
		}
	})

	// Slot 1-2: shows you do not need to implement a command handler if you do not have any commands.
	// The SDK returns an error for those slots.
}
