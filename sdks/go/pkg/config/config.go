package config

import "log/slog"

type RuntimeOptions struct {
	UseGrpc bool
	UseRest bool
	Server  ServerOptions
	Logger  LoggerOptions
}

type ServerOptions struct {
	// true for development mode, false for production mode
	IsDev bool

	// maximum number of concurrent connections (default: 100)
	MaxConnections int
}

type LoggerOptions struct {
	// AppName is used in log file naming
	AppName string

	// LogDir is the directory for log files
	LogDir string

	// Silent suppresses all log output
	Silent bool

	// Level is the minimum log level to output
	Level slog.Level

	// WriteToFile enables file logging
	WriteToFile bool

	// WriteToConsole enables console (stderr) logging
	WriteToConsole bool

	// UseJSON outputs logs in JSON format (useful for structured logging)
	UseJSON bool
}

func defaultRuntimeOptions() RuntimeOptions {
	return RuntimeOptions{
		UseGrpc: false,
		UseRest: false,
		Server:  DefaultServerOptions(),
		Logger:  DefaultLoggerOptions(),
	}
}

func DefaultServerOptions() ServerOptions {
	return ServerOptions{
		IsDev:          false,
		MaxConnections: 100,
	}
}

// DefaultLoggerOptions returns sensible defaults for release mode
func DefaultLoggerOptions() LoggerOptions {
	return LoggerOptions{
		AppName:        "catena",
		LogDir:         "./logs",
		Silent:         false,
		Level:          slog.LevelInfo, // INFO default in prod
		WriteToFile:    true,
		WriteToConsole: true,
		UseJSON:        false,
	}
}

// make sure RuntimeOptions implements slog.LogValuer for structured logging of the config
var _ slog.LogValuer = RuntimeOptions{}

func (o RuntimeOptions) LogValue() slog.Value {
	return slog.GroupValue(
		slog.Bool("dev", o.Server.IsDev),
		slog.Int("max_connections", o.Server.MaxConnections),
		slog.Group("logger",
			slog.String("app_name", o.Logger.AppName),
			slog.Bool("silent", o.Logger.Silent),
			slog.String("log_dir", o.Logger.LogDir),
			slog.String("level", o.Logger.Level.String()),
			slog.Bool("write_to_file", o.Logger.WriteToFile),
			slog.Bool("write_to_console", o.Logger.WriteToConsole),
			slog.Bool("use_json", o.Logger.UseJSON),
		),
	)
}
