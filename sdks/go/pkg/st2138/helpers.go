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

/**
 * @brief Helper functions for the proto builders.
 * @file helpers.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Keon Foster (keon.foster@rossvideo.com)
 * @date 2026-10-26
 */

package st2138

import "fmt"

// buildChild builds a child of a parent, returning the child and a bool indicating success.
// Short-circuits if the parent has an error. Propagates the child's error, if any, to the parent.
func buildChild[Builder any, Proto any](parentErr *error, child *Builder, build func(*Builder) (Proto, error), name string) (Proto, bool) {
	var zero Proto
	if *parentErr != nil {
		return zero, false
	}
	if child == nil {
		*parentErr = fmt.Errorf("%s: child cannot be nil", name)
		return zero, false
	}
	p, err := build(child)
	if err != nil {
		*parentErr = err
		return zero, false
	}
	return p, true
}

// put inserts val into m, allocating the map on first use.
func put[K comparable, V any](m *map[K]V, key K, val V) {
	if *m == nil {
		*m = map[K]V{}
	}
	(*m)[key] = val
}
