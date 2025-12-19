package catena

import (
	"net/http"

	"github.com/rossvideo/catena/build/go/protos"
)

// StatusResult represents an HTTP outcome (success or error) that the server can render uniformly.
type StatusResult struct {
	Status  int           // e.g., 200, 204, 400, 404, 405, 501, 500
	Payload *protos.Value // optional body to JSON-encode on success (2xx). If nil and Status is 204, no body.
	Message string        // optional error message for non-2xx statuses
}

// Convenience constructors
func OK(value any) StatusResult {
	return StatusResult{Status: http.StatusOK, Payload: ToProto(value)}
}
func NoContent() StatusResult { return StatusResult{Status: http.StatusNoContent} }
func BadRequest(msg string) StatusResult {
	return StatusResult{Status: http.StatusBadRequest, Message: msg}
}
func NotFound(msg string) StatusResult {
	return StatusResult{Status: http.StatusNotFound, Message: msg}
}
func MethodNotAllowed(msg string) StatusResult {
	return StatusResult{Status: http.StatusMethodNotAllowed, Message: msg}
}
func NotImplemented(msg string) StatusResult {
	return StatusResult{Status: http.StatusNotImplemented, Message: msg}
}
func InternalServerError(msg string) StatusResult {
	return StatusResult{Status: http.StatusInternalServerError, Message: msg}
}
