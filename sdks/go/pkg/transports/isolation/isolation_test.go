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

// Package isolation verifies the transport dependency boundary as a normal Go
// test: the REST transport must never transitively link the gRPC runtime, and
// the gRPC transport must never transitively link the REST-only JSON stack.
//
// It computes each package's full transitive import set via `go list -deps`
// (stdlib os/exec only) and asserts on sentinel dependencies. This runs as part
// of the ordinary `go test ./...` suite, so the guarantee is enforced wherever
// tests run -- no separate example binary or bespoke CI step required.
package isolation

import (
	"os/exec"
	"strings"
	"testing"
)

const (
	restPkg = "github.com/rossvideo/catena/sdks/go/pkg/transports/rest"
	grpcPkg = "github.com/rossvideo/catena/sdks/go/pkg/transports/grpc"

	// Sentinel dependencies used as proxies for each transport's stack:
	// gRPC's is the gRPC runtime; REST's is fastjson, its unique third-party
	// import. A "must not link" check is only meaningful while the opposite
	// transport still pulls its sentinel in (see TestSentinelsStillPresent).
	grpcMarker = "google.golang.org/grpc"
	restMarker = "github.com/valyala/fastjson"
)

// deps returns the transitive dependency import paths of pkg via `go list`.
func deps(t *testing.T, pkg string) []string {
	t.Helper()
	out, err := exec.Command("go", "list", "-deps", pkg).CombinedOutput()
	if err != nil {
		t.Fatalf("go list -deps %s failed: %v\n%s", pkg, err, out)
	}
	return strings.Fields(string(out))
}

// dependsOn reports whether deps contains module or any package beneath it.
func dependsOn(deps []string, module string) bool {
	for _, d := range deps {
		if d == module || strings.HasPrefix(d, module+"/") {
			return true
		}
	}
	return false
}

// TestRESTDoesNotLinkGRPC is the load-bearing guarantee: a REST-only consumer
// must never be forced to link the gRPC runtime.
func TestRESTDoesNotLinkGRPC(t *testing.T) {
	if dependsOn(deps(t, restPkg), grpcMarker) {
		t.Errorf("REST transport (%s) must not transitively depend on %s", restPkg, grpcMarker)
	}
}

// TestGRPCDoesNotLinkREST guards the reverse direction, using fastjson (REST's
// unique third-party dependency) as the sentinel for the REST/JSON stack.
func TestGRPCDoesNotLinkREST(t *testing.T) {
	if dependsOn(deps(t, grpcPkg), restMarker) {
		t.Errorf("gRPC transport (%s) must not transitively depend on %s", grpcPkg, restMarker)
	}
}

// TestSentinelsStillPresent guards the guards. Each isolation assertion above
// is only meaningful while the opposite transport actually pulls its sentinel
// in; if a sentinel disappears the paired check silently passes for the wrong
// reason. This fails loudly so the sentinel is refreshed in the same change.
// (Downgrade t.Errorf to t.Logf if you'd prefer a soft, non-fatal warning.)
func TestSentinelsStillPresent(t *testing.T) {
	if !dependsOn(deps(t, restPkg), restMarker) {
		t.Errorf("REST transport no longer depends on %s: the 'gRPC must not link REST' check is now vacuous -- choose a new REST-only sentinel", restMarker)
	}
	if !dependsOn(deps(t, grpcPkg), grpcMarker) {
		t.Errorf("gRPC transport no longer depends on %s: the 'REST must not link gRPC' check is now vacuous -- choose a new gRPC-only sentinel", grpcMarker)
	}
}
