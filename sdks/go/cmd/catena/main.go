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

// catena is the main executable for running Catena REST servers with plugin-based
// business logic. It loads plugins based on a YAML configuration file that defines
// which plugins go in which slots, along with their arguments.
//
// Usage:
//
//	catena config.yaml
//
// Or for simple cases without a config file:
//
//	catena plugin.so [plugin2.so ...]
//
// YAML Configuration Example:
//
//	slot_map:
//	  0:
//	    path: ./counter.so
//	    args:
//	      - name: --tls
//	        value: "on"
//	      - name: --static_root
//	        value: ./static
//	    trust:
//	      anchor: some_anchor
//	  16: ./dashboard.so  # simple string format also works
//	log_level: DEBUG
//	port: 8080
//
// Environment variables:
//
//	CATENA_PORT      HTTP server port (default 6254, overridden by config file)
//	CATENA_ENV       Environment: dev or prod (default prod)
//	CATENA_LOG_LEVEL Log level: debug, info, warning, error (overridden by config file)
package main

import (
	"flag"
	"fmt"
	"net/http"
	"os"
	"os/signal"
	"sort"
	"strings"
	"syscall"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/plugin"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"
)

func main() {
	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage: %s config.yaml\n", os.Args[0])
		fmt.Fprintf(os.Stderr, "   or: %s plugin.so [plugin2.so ...]\n\n", os.Args[0])
		fmt.Fprintf(os.Stderr, "Catena REST server with plugin-based business logic.\n\n")
		fmt.Fprintf(os.Stderr, "With YAML config, plugins are assigned to specific slots as defined.\n")
		fmt.Fprintf(os.Stderr, "Without config, plugins are assigned slots sequentially (0, 1, 2, ...).\n\n")
		fmt.Fprintf(os.Stderr, "Environment variables:\n")
		fmt.Fprintf(os.Stderr, "  CATENA_PORT       HTTP server port (default 6254)\n")
		fmt.Fprintf(os.Stderr, "  CATENA_ENV        Environment: dev or prod (default prod)\n")
		fmt.Fprintf(os.Stderr, "  CATENA_LOG_LEVEL  Log level: debug, info, warning, error\n")
		fmt.Fprintf(os.Stderr, "\nYAML config example:\n")
		fmt.Fprintf(os.Stderr, "  slot_map:\n")
		fmt.Fprintf(os.Stderr, "    0:\n")
		fmt.Fprintf(os.Stderr, "      path: ./counter.so\n")
		fmt.Fprintf(os.Stderr, "      args:\n")
		fmt.Fprintf(os.Stderr, "        - name: static_root\n")
		fmt.Fprintf(os.Stderr, "          value: ./static\n")
		fmt.Fprintf(os.Stderr, "        - name: tls\n")
		fmt.Fprintf(os.Stderr, "          value: on\n")
		fmt.Fprintf(os.Stderr, "    16: ./dashboard.so\n")
		fmt.Fprintf(os.Stderr, "  log_level: DEBUG\n")
		fmt.Fprintf(os.Stderr, "  port: 8080\n")
	}
	flag.Parse()

	args := flag.Args()
	if len(args) == 0 {
		fmt.Fprintf(os.Stderr, "Error: at least one argument is required (config.yaml or plugin.so)\n\n")
		flag.Usage()
		os.Exit(1)
	}

	// Determine if first argument is a YAML config or a plugin
	var yamlConfig *CatenaConfig
	var pluginEntries []pluginEntry

	if strings.HasSuffix(args[0], ".yaml") || strings.HasSuffix(args[0], ".yml") {
		// Load YAML configuration
		var err error
		yamlConfig, err = LoadConfig(args[0])
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error: %v\n", err)
			os.Exit(1)
		}

		// Convert slot_map to plugin entries
		for slot, entry := range yamlConfig.SlotMap {
			pluginEntries = append(pluginEntries, pluginEntry{
				slot: slot,
				path: entry.Path,
				args: entry.ToArgMap(),
			})
		}
	} else {
		// Legacy mode: plugins specified on command line, assigned sequential slots
		for i, path := range args {
			pluginEntries = append(pluginEntries, pluginEntry{
				slot: i,
				path: path,
				args: nil,
			})
		}
	}

	if len(pluginEntries) == 0 {
		fmt.Fprintf(os.Stderr, "Error: no plugins specified\n")
		os.Exit(1)
	}

	// Sort by slot for consistent initialization order
	sort.Slice(pluginEntries, func(i, j int) bool {
		return pluginEntries[i].slot < pluginEntries[j].slot
	})

	// Apply YAML config overrides to environment before SDK initialization
	if yamlConfig != nil {
		if yamlConfig.LogLevel != "" {
			os.Setenv("CATENA_LOG_LEVEL", yamlConfig.LogLevel)
		}
		if yamlConfig.Port > 0 {
			os.Setenv("CATENA_PORT", fmt.Sprintf("%d", yamlConfig.Port))
		}
	}

	// Initialize SDK
	cfg, err := catena.InitOptions(catena.Options{AppName: "catena"})
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error: failed to initialize SDK: %v\n", err)
		os.Exit(1)
	}
	defer catena.Close()

	// Setup shutdown channel
	shutdownChan := make(chan struct{})

	// Handle signals for graceful shutdown
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go func() {
		sig := <-sigChan
		logger.Info("Caught signal, shutting down", "signal", sig)
		close(shutdownChan)
	}()

	// Load plugins
	var loadedPlugins []*loadedPluginWithArgs
	slots := make([]int, 0, len(pluginEntries))

	for _, entry := range pluginEntries {
		logger.Info("Loading plugin", "path", entry.path, "slot", entry.slot)

		p, err := plugin.Load(entry.path)
		if err != nil {
			logger.Error("Failed to load plugin", "path", entry.path, "error", err)
			os.Exit(1)
		}
		p.Slot = entry.slot
		slots = append(slots, entry.slot)

		if p.Info.Name != "" {
			logger.Info("Plugin loaded",
				"name", p.Info.Name,
				"version", p.Info.Version,
				"slot", entry.slot)
		}

		loadedPlugins = append(loadedPlugins, &loadedPluginWithArgs{
			LoadedPlugin: p,
			args:         entry.args,
		})
	}

	// Initialize plugins
	for _, lp := range loadedPlugins {
		if lp.Init != nil {
			ctx := &plugin.Context{
				Config:       cfg,
				ShutdownChan: shutdownChan,
				Slot:         lp.Slot,
				Args:         lp.args,
			}
			logger.Info("Initializing plugin", "path", lp.Path, "slot", lp.Slot, "args", lp.args)
			if err := lp.Init(ctx); err != nil {
				logger.Error("Plugin Init failed", "path", lp.Path, "error", err)
				os.Exit(1)
			}
		}
	}

	// Create server with all assigned slots
	srv := rest.NewServer(slots)

	// Register handlers for each plugin
	var fallbackPlugin *loadedPluginWithArgs
	for _, lp := range loadedPlugins {
		registerPluginHandlers(srv, lp.LoadedPlugin)
		if lp.Fallback != nil {
			fallbackPlugin = lp
		}
	}

	// Register fallback handler (last plugin with a fallback wins)
	if fallbackPlugin != nil {
		srv.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
			return fallbackPlugin.Fallback(w, r)
		})
	}

	// Start server
	logger.Info("=======================================================")
	logger.Info("Catena REST Server")
	logger.Info("=======================================================")
	logger.Info("Server starting", "port", cfg.Port)
	for _, lp := range loadedPlugins {
		name := lp.Path
		if lp.Info.Name != "" {
			name = lp.Info.Name
		}
		logger.Info("  Plugin", "name", name, "slot", lp.Slot)
	}
	logger.Info("=======================================================")

	go func() {
		if err := srv.Start(cfg.Port); err != nil {
			logger.Error("Server failed", "error", err)
			os.Exit(1)
		}
	}()

	// Wait for shutdown signal
	<-shutdownChan

	// Call Shutdown on all plugins (in reverse order)
	for i := len(loadedPlugins) - 1; i >= 0; i-- {
		lp := loadedPlugins[i]
		if lp.Shutdown != nil {
			logger.Info("Shutting down plugin", "path", lp.Path)
			if err := lp.Shutdown(); err != nil {
				logger.Error("Plugin Shutdown failed", "path", lp.Path, "error", err)
			}
		}
	}

	srv.Shutdown()
	logger.Info("Server shutdown complete")
}

// pluginEntry holds info about a plugin to be loaded
type pluginEntry struct {
	slot int
	path string
	args map[string]string
}

// loadedPluginWithArgs wraps a LoadedPlugin with its config args
type loadedPluginWithArgs struct {
	*plugin.LoadedPlugin
	args map[string]string
}

// registerPluginHandlers registers all of a plugin's handlers with the server for its assigned slot
func registerPluginHandlers(srv *rest.Server, p *plugin.LoadedPlugin) {
	slot := p.Slot

	if p.GetDevice != nil {
		srv.RegisterGetDeviceHandler(slot, p.GetDevice)
	} else {
		logger.Warning("Plugin has no GetDevice handler", "path", p.Path, "slot", slot)
	}

	if p.GetValue != nil {
		srv.RegisterGetValueHandler(slot, p.GetValue)
	} else {
		logger.Warning("Plugin has no GetValue handler", "path", p.Path, "slot", slot)
	}

	if p.SetValue != nil {
		srv.RegisterSetValueHandler(slot, p.SetValue)
	} else {
		logger.Warning("Plugin has no SetValue handler", "path", p.Path, "slot", slot)
	}

	if p.GetAsset != nil {
		srv.RegisterGetAssetHandler(slot, p.GetAsset)
	} else {
		logger.Warning("Plugin has no GetAsset handler", "path", p.Path, "slot", slot)
	}

	if p.ExecuteCommand != nil {
		srv.RegisterExecuteCommandHandler(slot, p.ExecuteCommand)
	} else {
		logger.Warning("Plugin has no ExecuteCommand handler", "path", p.Path, "slot", slot)
	}

	if p.Fallback != nil {
		srv.RegisterFallbackHandler(p.Fallback)
	} else {
		logger.Warning("Plugin has no Fallback handler", "path", p.Path, "slot", slot)
	}
}
