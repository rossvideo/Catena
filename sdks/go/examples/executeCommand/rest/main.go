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
	"net/http"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	// "github.com/rossvideo/catena/sdks/go/pkg/rest"

	executecommand "github.com/rossvideo/catena/sdks/go/examples/executeCommand"
)

//go:embed static/*
var staticFS embed.FS

func main() {
	executecommand.RunExample("executeCommand_REST", func(slots []uint16, cfg catena.Config) catena.CatenaServer {
		srv := rest.NewServer(slots, 100)
		srv.RegisterFallbackHandler(func(w http.ResponseWriter, r *http.Request) (catena.CatenaValue, catena.StatusResult) {
			if r.URL.Path == "/" {
				data, err := staticFS.ReadFile("static/index.htm")
				if err != nil {
					return catena.ReplyError[catena.CatenaValue](catena.StatusCodeNotFound, "index.html not found")
				}
				w.Header().Set("Content-Type", "text/html; charset=utf-8")
				w.Write(data)
				return catena.Reply(catena.CatenaValue{})
			}
			return catena.ReplyError[catena.CatenaValue](catena.StatusCodeNotFound, "endpoint not found: "+r.URL.Path)
		})
		return srv
	})
}
