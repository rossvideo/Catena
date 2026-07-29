package main

import (
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
)

func registerCommandHandler(srv catena.Server, counter *CounterState, broadcastRunning func(), counterScope string) {
	// Slot 0: switch per command FQOID and build command responses manually.
	// This handler does not reduce allowed scopes beyond the base level of
	// any command scope (mon, op, conf, or adm), however it broadcasts updates
	// to the counter value only to specified counterScope.
	srv.RegisterExecuteCommandHandler(0, func(slot uint16, commandFqoid string, payload any, respond bool, ctx catena.HandlerContext, stream catena.Stream[st2138.CommandResponse]) catena.StatusResult {
		logger.Info("ExecuteCommand", "slot", slot, "command", commandFqoid)
		if !ctx.HasWriteScope(counterScope) {
			logger.Warning("Unauthorized command execution attempt", "slot", slot, "command", commandFqoid)
			return catena.StatusWithCode(catena.StatusCodePermissionDenied, "Caller does not have required scope for this command")
		}

		// sendCounter emits the current counter value as a command response.
		// When the caller opted out (respond=false) it skips the Send entirely -
		// the transport would discard it anyway, so a smart handler saves the work.
		sendCounter := func() catena.StatusResult {
			if respond {
				val, _ := st2138.ToValue(counter.GetValue())
				if err := stream.Send(st2138.CommandValue(val)); err != nil {
					return catena.StatusWithCode(catena.StatusCodeInternal, err.Error())
				}
			}
			return catena.StatusWithCode(catena.StatusCodeOk, "")
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
			return sendCounter()
		case "stop":
			if !counter.IsRunning() {
				logger.Info("Stop command - already stopped")
			} else {
				counter.Stop()
				logger.Info("Counter stopped", "value", counter.GetValue())
				broadcastRunning()
			}
			srv.BroadcastUpdate(0, "counter", counter.GetValue(), counterScope)
			return sendCounter()
		case "add10":
			counter.Add(10)
			logger.Info("Added 10 to counter", "value", counter.GetValue())
			srv.BroadcastUpdate(0, "counter", counter.GetValue(), counterScope)
			return sendCounter()
		case "reset":
			counter.Reset()
			logger.Info("Counter reset", "value", counter.GetValue())
			srv.BroadcastUpdate(0, "counter", counter.GetValue(), counterScope)
			return sendCounter()
		default:
			logger.Warning("Command not found", "slot", slot, "command", commandFqoid)
			return catena.StatusWithCode(catena.StatusCodeNotFound, "Command not found: "+commandFqoid)
		}
	})

	// Slot 1-2: shows you do not need to implement a command handler if you do not have any commands.
	// The SDK returns an error for those slots.
}
