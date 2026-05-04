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

package main

import (
	"embed"
	"encoding/json"
	"fmt"
	"net/http"
	"sync"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/rest"

	oneofeverything "github.com/rossvideo/catena/sdks/go/examples/oneOfEverything"
)

//go:embed webui/*
var webFS embed.FS

func main() {
	assets := &sync.Map{}
	payloads, _ := catena.LoadPayloadsFromEmbed(oneofeverything.StaticFS, "static")
	for id, payload := range payloads {
		assets.Store(id, payload)
	}

	oneofeverything.RunExample("oneOfEverything_REST", func(slots []uint16, cfg catena.Config) catena.CatenaServer {
		srv := rest.NewServer(slots, 100)
		srv.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
			if r.URL.Path == "/assets-list" {
				var assetList []map[string]any
				assets.Range(func(key, value any) bool {
					payload := value.(catena.DataPayload)
					assetList = append(assetList, map[string]any{
						"id":           key.(string),
						"content_type": payload.Metadata["content-type"],
						"file_name":    payload.Metadata["file-name"],
						"size":         len(payload.Payload),
					})
					return true
				})
				w.Header().Set("Content-Type", "application/json")
				json.NewEncoder(w).Encode(assetList)
				return catena.Reply(catena.CatenaValue{})
			}

			fileMap := map[string]struct {
				path        string
				contentType string
			}{
				"/":           {"webui/index.htm", "text/html; charset=utf-8"},
				"/styles.css": {"webui/styles.css", "text/css; charset=utf-8"},
				"/script.js":  {"webui/script.js", "application/javascript; charset=utf-8"},
			}

			if file, ok := fileMap[r.URL.Path]; ok {
				data, err := webFS.ReadFile(file.path)
				if err != nil {
					return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "file not found: "+r.URL.Path)
				}
				w.Header().Set("Content-Type", file.contentType)
				w.Write(data)
				return catena.Reply(catena.CatenaValue{})
			}
			return catena.ReplyError[catena.CatenaValue](catena.NOT_FOUND, "endpoint not found: "+r.URL.Path)
		})
		return srv
	},
		func(port int) {
			logger.Info("=======================================================")
			logger.Info("One of Everything REST Example")
			logger.Info("=======================================================")
			logger.Info("REST server starting", "port", port)
			logger.Info("")
			logger.Info("Web UI available at:")
			logger.Info(fmt.Sprintf("  http://localhost:%d/", port))
			logger.Info("")
			logger.Info("=======================================================")
		},
	)
}
