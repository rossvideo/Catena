package com.rossvideo.catena.command;

import catena.core.parameter.CommandResponse;
import catena.core.parameter.ExecuteCommandPayload;

public interface SimpleCommandHandler
{
    public CommandResponse processCommand(ExecuteCommandPayload commandExecution) throws Exception;
}
