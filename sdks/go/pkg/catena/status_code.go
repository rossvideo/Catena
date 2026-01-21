package catena

import (
	"net/http"
)

// StatusCode represents both gRPC and HTTP status codes.
// Codes 0-16 are compatible with gRPC status codes.
// Codes 200+ are HTTP-specific and should not be used in gRPC contexts.
//
// Based on Google's gRPC project (Apache 2.0 license):
// https://github.com/grpc/grpc
// http://www.apache.org/licenses/LICENSE-2.0
type StatusCode int

type StatusResult struct {
	Code  StatusCode
	Error string
}

const (
	// gRPC-compatible status codes (0-16)

	// OK indicates the operation completed successfully.
	OK StatusCode = 0

	// CANCELLED indicates the operation was cancelled (typically by the caller).
	CANCELLED StatusCode = 1

	// UNKNOWN indicates an unknown error occurred.
	UNKNOWN StatusCode = 2

	// INVALID_ARGUMENT indicates the client specified an invalid argument.
	INVALID_ARGUMENT StatusCode = 3

	// DEADLINE_EXCEEDED indicates the deadline expired before operation could complete.
	DEADLINE_EXCEEDED StatusCode = 4

	// NOT_FOUND indicates some requested entity was not found.
	NOT_FOUND StatusCode = 5

	// ALREADY_EXISTS indicates the entity that we attempted to create already exists.
	ALREADY_EXISTS StatusCode = 6

	// PERMISSION_DENIED indicates the caller does not have permission to execute the operation.
	PERMISSION_DENIED StatusCode = 7

	// RESOURCE_EXHAUSTED indicates some resource has been exhausted.
	RESOURCE_EXHAUSTED StatusCode = 8

	// FAILED_PRECONDITION indicates the operation was rejected because the system is not
	// in a state required for the operation's execution.
	FAILED_PRECONDITION StatusCode = 9

	// ABORTED indicates the operation was aborted, typically due to a concurrency issue.
	ABORTED StatusCode = 10

	// OUT_OF_RANGE indicates the operation was attempted past the valid range.
	OUT_OF_RANGE StatusCode = 11

	// UNIMPLEMENTED indicates the operation is not implemented or not supported.
	UNIMPLEMENTED StatusCode = 12

	// INTERNAL indicates internal errors. Something is very broken.
	INTERNAL StatusCode = 13

	// UNAVAILABLE indicates the service is currently unavailable.
	UNAVAILABLE StatusCode = 14

	// DATA_LOSS indicates unrecoverable data loss or corruption.
	DATA_LOSS StatusCode = 15

	// UNAUTHENTICATED indicates the request does not have valid authentication credentials.
	UNAUTHENTICATED StatusCode = 16

	// REST-specific status codes (do not use in gRPC contexts)

	// CREATED indicates the request has succeeded and a new resource has been created.
	CREATED StatusCode = 201

	// ACCEPTED indicates the request has been accepted for processing.
	ACCEPTED StatusCode = 202

	// NO_CONTENT indicates the server successfully processed the request with no content.
	NO_CONTENT StatusCode = 204

	// METHOD_NOT_ALLOWED indicates the method is not allowed for the requested resource.
	METHOD_NOT_ALLOWED StatusCode = 405

	// CONFLICT indicates the request could not be completed due to a conflict.
	CONFLICT StatusCode = 409

	// UNPROCESSABLE_ENTITY indicates the request was well-formed but semantically erroneous.
	UNPROCESSABLE_ENTITY StatusCode = 422

	// TOO_MANY_REQUESTS indicates the user has sent too many requests.
	TOO_MANY_REQUESTS StatusCode = 429

	// BAD_GATEWAY indicates the server received an invalid response from upstream.
	BAD_GATEWAY StatusCode = 502

	// SERVICE_UNAVAILABLE indicates the server is not ready to handle the request.
	SERVICE_UNAVAILABLE StatusCode = 503

	// GATEWAY_TIMEOUT indicates the server did not receive a timely response from upstream.
	GATEWAY_TIMEOUT StatusCode = 504
)

// ToHTTPStatus converts a StatusCode to an HTTP status code.
// gRPC codes are mapped to their closest HTTP equivalents.
func (s StatusCode) ToHTTPStatus() int {
	switch s {
	case OK:
		return http.StatusOK
	case CANCELLED:
		return 499 // Client Closed Request (nginx convention)
	case UNKNOWN:
		return http.StatusInternalServerError
	case INVALID_ARGUMENT:
		return http.StatusBadRequest
	case DEADLINE_EXCEEDED:
		return http.StatusGatewayTimeout
	case NOT_FOUND:
		return http.StatusNotFound
	case ALREADY_EXISTS:
		return http.StatusConflict
	case PERMISSION_DENIED:
		return http.StatusForbidden
	case RESOURCE_EXHAUSTED:
		return http.StatusTooManyRequests
	case FAILED_PRECONDITION:
		return http.StatusBadRequest
	case ABORTED:
		return http.StatusConflict
	case OUT_OF_RANGE:
		return http.StatusBadRequest
	case UNIMPLEMENTED:
		return http.StatusNotImplemented
	case INTERNAL:
		return http.StatusInternalServerError
	case UNAVAILABLE:
		return http.StatusServiceUnavailable
	case DATA_LOSS:
		return http.StatusInternalServerError
	case UNAUTHENTICATED:
		return http.StatusUnauthorized
	// REST codes map directly
	case CREATED, ACCEPTED, NO_CONTENT, METHOD_NOT_ALLOWED, CONFLICT,
		UNPROCESSABLE_ENTITY, TOO_MANY_REQUESTS, BAD_GATEWAY,
		SERVICE_UNAVAILABLE, GATEWAY_TIMEOUT:
		return int(s)
	default:
		return http.StatusInternalServerError
	}
}

// Reply helper functions that return (CatenaValue, StatusResult)
// CatenaValue is returned by value (never nil), but its internal *protos.Value can be nil.

// ReplyOK returns a successful response with the given value.
func ReplyOK(value CatenaValue) (CatenaValue, StatusResult) {
	return value, StatusResult{Code: OK}
}

// ReplyCreated returns a 201 Created response with the given value.
func ReplyCreated(value CatenaValue) (CatenaValue, StatusResult) {
	return value, StatusResult{Code: CREATED}
}

// ReplyAccepted returns a 202 Accepted response with the given value.
func ReplyAccepted(value CatenaValue) (CatenaValue, StatusResult) {
	return value, StatusResult{Code: ACCEPTED}
}

// ReplyNoContent returns a 204 No Content response with no payload.
func ReplyNoContent() (CatenaValue, StatusResult) {
	return CatenaValue{Value: nil}, StatusResult{Code: NO_CONTENT}
}

// ReplyBadRequest returns a 400 Bad Request error with an optional message.
func ReplyBadRequest(msg string) (CatenaValue, StatusResult) {
	if msg == "" {
		return CatenaValue{Value: nil}, StatusResult{Code: INVALID_ARGUMENT}
	}
	return CatenaValue{Value: nil}, StatusResult{Code: INVALID_ARGUMENT, Error: msg}
}

// ReplyNotFound returns a 404 Not Found error with an optional message.
func ReplyNotFound(msg string) (CatenaValue, StatusResult) {
	if msg == "" {
		return CatenaValue{Value: nil}, StatusResult{Code: NOT_FOUND}
	}
	return CatenaValue{Value: nil}, StatusResult{Code: NOT_FOUND, Error: msg}
}

// ReplyMethodNotAllowed returns a 405 Method Not Allowed error with an optional message.
func ReplyMethodNotAllowed(msg string) (CatenaValue, StatusResult) {
	if msg == "" {
		return CatenaValue{Value: nil}, StatusResult{Code: METHOD_NOT_ALLOWED}
	}
	return CatenaValue{Value: nil}, StatusResult{Code: METHOD_NOT_ALLOWED, Error: msg}
}

// ReplyConflict returns a 409 Conflict error with an optional message.
func ReplyConflict(msg string) (CatenaValue, StatusResult) {
	if msg == "" {
		return CatenaValue{Value: nil}, StatusResult{Code: CONFLICT}
	}
	return CatenaValue{Value: nil}, StatusResult{Code: CONFLICT, Error: msg}
}

// ReplyUnprocessableEntity returns a 422 Unprocessable Entity error with an optional message.
func ReplyUnprocessableEntity(msg string) (CatenaValue, StatusResult) {
	if msg == "" {
		return CatenaValue{Value: nil}, StatusResult{Code: UNPROCESSABLE_ENTITY}
	}
	return CatenaValue{Value: nil}, StatusResult{Code: UNPROCESSABLE_ENTITY, Error: msg}
}

// ReplyNotImplemented returns a 501 Not Implemented error with an optional message.
func ReplyNotImplemented(msg string) (CatenaValue, StatusResult) {
	if msg == "" {
		return CatenaValue{Value: nil}, StatusResult{Code: UNIMPLEMENTED}
	}
	return CatenaValue{Value: nil}, StatusResult{Code: UNIMPLEMENTED, Error: msg}
}

// ReplyInternalError returns a 500 Internal Server Error with an optional message.
func ReplyInternalError(msg string) (CatenaValue, StatusResult) {
	if msg == "" {
		return CatenaValue{Value: nil}, StatusResult{Code: INTERNAL}
	}
	return CatenaValue{Value: nil}, StatusResult{Code: INTERNAL, Error: msg}
}

// ReplyUnavailable returns a 503 Service Unavailable error with an optional message.
func ReplyUnavailable(msg string) (CatenaValue, StatusResult) {
	if msg == "" {
		return CatenaValue{Value: nil}, StatusResult{Code: UNAVAILABLE}
	}
	return CatenaValue{Value: nil}, StatusResult{Code: UNAVAILABLE, Error: msg}
}

// ReplyUnauthenticated returns a 401 Unauthorized error with an optional message.
func ReplyUnauthenticated(msg string) (CatenaValue, StatusResult) {
	if msg == "" {
		return CatenaValue{Value: nil}, StatusResult{Code: UNAUTHENTICATED}
	}
	return CatenaValue{Value: nil}, StatusResult{Code: UNAUTHENTICATED, Error: msg}
}

// ReplyPermissionDenied returns a 403 Forbidden error with an optional message.
func ReplyPermissionDenied(msg string) (CatenaValue, StatusResult) {
	if msg == "" {
		return CatenaValue{Value: nil}, StatusResult{Code: PERMISSION_DENIED}
	}
	return CatenaValue{Value: nil}, StatusResult{Code: PERMISSION_DENIED, Error: msg}
}

// ReplyWithCode returns a response with the given StatusCode and optional message.
// This is a generic helper for cases not covered by specific Reply* functions.
func ReplyWithCode(code StatusCode, msg string) (CatenaValue, StatusResult) {
	if msg == "" {
		return CatenaValue{Value: nil}, StatusResult{Code: code}
	}
	return CatenaValue{Value: nil}, StatusResult{Code: code, Error: msg}
}
