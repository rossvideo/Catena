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
 * @brief gRPC transport for the Catena SDK.
 * @file grpc_transport.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @author Nelson Daniels (nelson.daniels@rossvideo.com)
 * @author Andrew Brown (andrew.brown@rossvideo.com)
 * @date 2026-05-11
 */

package grpc

import (
	"context"
	"errors"
	"fmt"
	"maps"
	"net"

	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
	"github.com/rossvideo/catena/sdks/go/pkg/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/protos/rpc"
	"github.com/rossvideo/catena/sdks/go/pkg/st2138"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/reflection"
	"google.golang.org/grpc/status"
)

type Transport struct {
	catenaService *catenaService
	grpcServer    *grpc.Server
	listener      net.Listener
	runtime       catena.ServerRuntime

	port       int
	reflection bool
}

var _ catena.Transport = (*Transport)(nil)

// NewTransport creates a new gRPC transport with the given configuration.
func NewTransport(cfg Options) *Transport {
	transport := &Transport{
		catenaService: &catenaService{},
		port:          cfg.Port,
		reflection:    cfg.Reflection,
	}
	transport.catenaService.transport = transport
	transport.grpcServer = grpc.NewServer(
		grpc.UnaryInterceptor(transport.unaryInterceptor),
		grpc.StreamInterceptor(transport.streamInterceptor),
	)

	rpc.RegisterCatenaServiceServer(transport.grpcServer, transport.catenaService)

	if cfg.Reflection {
		reflection.Register(transport.grpcServer)
		logger.Info("gRPC server created with reflection enabled")
	} else {
		logger.Info("gRPC server created")
	}
	return transport
}

func (t *Transport) Start(ctx context.Context, runtime catena.ServerRuntime) error {
	t.runtime = runtime

	if t.listener == nil {
		addr := fmt.Sprintf(":%d", t.port)
		listener, err := net.Listen("tcp", addr)
		if err != nil {
			logger.Error("Failed to create listener", "address", addr, "error", err)
			return fmt.Errorf("failed to listen on %s: %w", addr, err)
		}
		t.listener = listener
	}

	logger.Info("Starting gRPC transport", "address", t.listener.Addr().String())
	logger.Info("grpc listening and ready to accept connections")

	go func() {
		if err := t.grpcServer.Serve(t.listener); err != nil {
			// already closed is not an error condition worth logging as an error
			if !errors.Is(err, net.ErrClosed) {
				logger.Error("gRPC server error", "error", err)
			}
		}
	}()
	return nil
}

func (t *Transport) Shutdown(ctx context.Context) error {
	logger.Info("Shutting down gRPC transport")

	// Gracefully stop the gRPC server in a goroutine so we can also listen for context
	// cancellation. GracefulStop will wait for existing RPCs to finish but will stop
	// accepting new connections. If the context is cancelled before it finishes, we force
	// stop the server to avoid hanging indefinitely.
	done := make(chan struct{})
	go func() {
		t.grpcServer.GracefulStop()
		close(done)
	}()

	// While waiting for the gRPC server to stop in the goroutine, signal the runtime
	// to shut down any active connections owned by this transport. This allows
	// GracefulStop to actually stop, gracefully.
	t.runtime.ShutdownTransportConnections(ctx, t)

	var shutdownErr error
	select {
	case <-done:
		logger.Info("gRPC server stopped gracefully")
	case <-ctx.Done():
		logger.Warning("gRPC shutdown timed out, forcing stop")
		shutdownErr = ctx.Err()
		t.grpcServer.Stop()
		<-done
	}

	// Close the listener if it's still open
	if t.listener != nil {
		if err := t.listener.Close(); err != nil {
			if !errors.Is(err, net.ErrClosed) {
				logger.Error("Failed to close gRPC listener", "error", err)
				return fmt.Errorf("failed to close listener: %w", err)
			}
		} else {
			logger.Info("gRPC listener closed")
		}
		t.listener = nil
	}
	return shutdownErr
}

// sanitizeGRPCError replaces detailed error messages with generic status code
// descriptions in production mode, matching the REST server's IsDev behavior.
func (t *Transport) sanitizeGRPCError(err error) error {
	if err == nil {
		return nil
	}

	if t != nil && t.runtime != nil && t.runtime.IsDev() {
		return err
	}

	st, ok := status.FromError(err)
	if !ok {
		return status.Error(codes.Unknown, codes.Unknown.String())
	}
	return status.Error(st.Code(), st.Code().String())
}

// unaryInterceptor logs all incoming unary RPC calls
func (t *Transport) unaryInterceptor(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
	logger.Debug("gRPC unary call received", "method", info.FullMethod)
	resp, err := handler(ctx, req)
	if err != nil {
		logger.Error("gRPC unary call error", "method", info.FullMethod, "error", err)
	}
	return resp, t.sanitizeGRPCError(err)
}

// streamInterceptor logs all incoming streaming RPC calls
func (t *Transport) streamInterceptor(srv interface{}, ss grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
	logger.Debug("gRPC stream call received", "method", info.FullMethod, "isClientStream", info.IsClientStream, "isServerStream", info.IsServerStream)
	err := handler(srv, ss)
	if err != nil {
		// Context cancellation is normal when clients disconnect - log at debug level
		if status.Code(err) == codes.Canceled || err == context.Canceled || err == context.DeadlineExceeded {
			logger.Debug("gRPC stream ended", "method", info.FullMethod, "reason", err)
		} else {
			logger.Error("gRPC stream call error", "method", info.FullMethod, "error", err)
		}
	}
	return t.sanitizeGRPCError(err)
}

func (t *Transport) retrieveMetadataFromContext(ctx context.Context) catena.TransportContext {
	var accessToken string
	requestMetadata, ok := metadata.FromIncomingContext(ctx)
	metadataMap := make(map[string][]string)
	if ok {
		if vals := requestMetadata.Get("authorization"); len(vals) > 0 {
			accessToken = vals[0]
		} else {
			logger.Debug("No authorization metadata found in context")
		}

		// Convert metadata.MD map[string][]string to map[string][]string
		metadataMap = maps.Clone(requestMetadata)
	} else {
		logger.Debug("No metadata found in context")
	}

	transportContext := catena.TransportContext{
		AccessToken: accessToken,
		Metadata:    metadataMap,
		Ctx:         ctx,
	}

	return transportContext
}

// struct to hold the endpoint implementations for the gRPC service. We embed the unimplemented server
type catenaService struct {
	transport *Transport
	rpc.UnimplementedCatenaServiceServer
}

func (s *catenaService) GetPopulatedSlots(ctx context.Context, req *protos.Empty) (*protos.SlotList, error) {
	logger.Info("GetPopulatedSlots")

	transportContext := s.transport.retrieveMetadataFromContext(ctx)
	slots, res := s.transport.runtime.GetSlots(transportContext)
	if res.Code != catena.StatusCodeOk {
		return nil, status.Error(ToGRPCCode(res.Code), res.Error)
	}

	uint32slots := make([]uint32, len(slots))
	for i, slot := range slots {
		uint32slots[i] = uint32(slot)
	}
	return &protos.SlotList{Slots: uint32slots}, nil
}

// DeviceRequest streams the device information for a slot
func (s *catenaService) DeviceRequest(req *protos.DeviceRequestPayload, stream grpc.ServerStreamingServer[protos.DeviceComponent]) error {
	slot, err := catena.ValidateSlot(req.Slot)
	if err.Code != catena.StatusCodeOk {
		return status.Error(ToGRPCCode(err.Code), err.Error)
	}

	logger.Info("DeviceRequest", "slot", slot)

	transportContext := s.transport.retrieveMetadataFromContext(stream.Context())
	device, res := s.transport.runtime.InvokeGetDeviceHandler(slot, transportContext)
	if res.IsError() {
		logger.Error("DeviceRequest handler error", "slot", slot, "error", res.Error)
		return status.Error(ToGRPCCode(res.Code), res.Error)
	}

	protoDevice := device.Proto
	if protoDevice == nil {
		logger.Error("DeviceRequest device returned nil", "slot", slot)
		return status.Error(codes.Internal, "device returned nil")
	}

	// Wrap the device in a DeviceComponent
	component := &protos.DeviceComponent{
		Kind: &protos.DeviceComponent_Device{
			Device: protoDevice,
		},
	}

	return stream.Send(component)
}

// GetValue returns the value of a parameter
func (s *catenaService) GetValue(ctx context.Context, req *protos.GetValuePayload) (*protos.Value, error) {
	slot, err := catena.ValidateSlot(req.Slot)
	if err.Code != catena.StatusCodeOk {
		return nil, status.Error(ToGRPCCode(err.Code), err.Error)
	}

	fqoid := req.Oid
	logger.Info("GetValue", "slot", slot, "fqoid", fqoid)

	transportContext := s.transport.retrieveMetadataFromContext(ctx)
	value, result := s.transport.runtime.InvokeGetValueHandler(slot, fqoid, transportContext)
	if result.IsError() {
		logger.Error("GetValue handler error", "slot", slot, "fqoid", fqoid, "error", result.Error)
		return nil, status.Error(ToGRPCCode(result.Code), result.Error)
	}

	return value.Proto, nil
}

// SetValue sets the value of a parameter
func (s *catenaService) SetValue(ctx context.Context, req *protos.SingleSetValuePayload) (*protos.Empty, error) {
	slot, err := catena.ValidateSlot(req.Slot)
	if err.Code != catena.StatusCodeOk {
		return nil, status.Error(ToGRPCCode(err.Code), err.Error)
	}

	if req.Value == nil {
		logger.Error("SetValue nil value payload", "slot", slot)
		return nil, status.Error(codes.InvalidArgument, "value payload is nil")
	}

	fqoid := req.Value.Oid
	logger.Info("SetValue", "slot", slot, "fqoid", fqoid)

	// Convert proto value to native Go type
	nativeValue, errProto := st2138.FromProto(req.Value.Value)
	if errProto != nil {
		logger.Error("SetValue failed to convert proto value", "slot", slot, "fqoid", fqoid, "error", errProto)
		return nil, status.Error(codes.InvalidArgument, fmt.Sprintf("invalid value: %v", errProto))
	}

	transportContext := s.transport.retrieveMetadataFromContext(ctx)
	entries := []catena.SetValueEntry{{Fqoid: fqoid, Value: nativeValue}}
	result := s.transport.runtime.InvokeSetValueHandler(slot, entries, transportContext)
	if result.IsError() {
		logger.Error("SetValue handler error", "slot", slot, "fqoid", fqoid, "error", result.Error)
		return nil, status.Error(ToGRPCCode(result.Code), result.Error)
	}

	return &protos.Empty{}, nil
}

// MultiSetValue sets multiple parameter values
func (s *catenaService) MultiSetValue(ctx context.Context, req *protos.MultiSetValuePayload) (*protos.Empty, error) {
	slot, err := catena.ValidateSlot(req.Slot)
	if err.Code != catena.StatusCodeOk {
		return nil, status.Error(ToGRPCCode(err.Code), err.Error)
	}

	logger.Info("MultiSetValue", "slot", slot, "count", len(req.Values))
	transportContext := s.transport.retrieveMetadataFromContext(ctx)

	entries := make([]catena.SetValueEntry, 0, len(req.Values))
	for _, setValue := range req.Values {
		fqoid := setValue.Oid

		nativeValue, errProto := st2138.FromProto(setValue.Value)
		if errProto != nil {
			logger.Error("MultiSetValue failed to convert proto value", "slot", slot, "fqoid", fqoid, "error", errProto)
			return nil, status.Error(codes.InvalidArgument, fmt.Sprintf("invalid value for %s: %v", fqoid, errProto))
		}

		entries = append(entries, catena.SetValueEntry{Fqoid: fqoid, Value: nativeValue})
	}

	result := s.transport.runtime.InvokeSetValueHandler(slot, entries, transportContext)
	if result.IsError() {
		logger.Error("MultiSetValue handler error", "slot", slot, "error", result.Error)
		return nil, status.Error(ToGRPCCode(result.Code), result.Error)
	}

	return &protos.Empty{}, nil
}

// ExternalObjectRequest streams external object (asset) data
func (s *catenaService) ExternalObjectRequest(req *protos.ExternalObjectRequestPayload, stream grpc.ServerStreamingServer[protos.ExternalObjectPayload]) error {
	slot, err := catena.ValidateSlot(req.Slot)
	if err.Code != catena.StatusCodeOk {
		return status.Error(ToGRPCCode(err.Code), err.Error)
	}

	fqoid := req.Oid
	logger.Info("ExternalObjectRequest", "slot", slot, "fqoid", fqoid)

	transportContext := s.transport.retrieveMetadataFromContext(stream.Context())

	asset, result := s.transport.runtime.InvokeReadAssetHandler(slot, fqoid, transportContext)
	if result.IsError() {
		logger.Error("ExternalObjectRequest handler error", "slot", slot, "fqoid", fqoid, "error", result.Error)
		return status.Error(ToGRPCCode(result.Code), result.Error)
	}

	protoAsset := asset.Proto
	if protoAsset == nil {
		logger.Error("ExternalObjectRequest asset returned nil", "slot", slot, "fqoid", fqoid)
		return status.Error(codes.Internal, "asset returned nil")
	}

	return stream.Send(protoAsset)
}

// ExecuteCommand executes a command and streams the response
func (s *catenaService) ExecuteCommand(req *protos.ExecuteCommandPayload, stream grpc.ServerStreamingServer[protos.CommandResponse]) error {
	slot, err := catena.ValidateSlot(req.Slot)
	if err.Code != catena.StatusCodeOk {
		return status.Error(ToGRPCCode(err.Code), err.Error)
	}

	commandFqoid := req.Oid
	logger.Info("ExecuteCommand", "slot", slot, "command", commandFqoid)

	var payload any
	if req.Value != nil {
		var errProto error
		payload, errProto = st2138.FromProto(req.Value)
		if errProto != nil {
			logger.Error("ExecuteCommand failed to convert payload", "slot", slot, "command", commandFqoid, "error", errProto)
			return status.Error(codes.InvalidArgument, fmt.Sprintf("invalid command payload: %v", errProto))
		}
	}

	transportContext := s.transport.retrieveMetadataFromContext(stream.Context())

	adapter := &grpcStream[st2138.CommandResponse]{ss: stream}
	result := s.transport.runtime.InvokeExecuteCommandHandler(slot, commandFqoid, payload, req.Respond, adapter, transportContext)
	if result.IsError() {
		logger.Error("ExecuteCommand handler error", "slot", slot, "command", commandFqoid, "error", result.Error)
		return status.Error(ToGRPCCode(result.Code), result.Error)
	}

	return nil
}

// GetParam returns a single parameter (metadata + value)
func (s *catenaService) GetParam(ctx context.Context, req *protos.GetParamPayload) (*protos.DeviceComponent_ComponentParam, error) {
	slot, err := catena.ValidateSlot(req.Slot)
	if err.Code != catena.StatusCodeOk {
		return nil, status.Error(ToGRPCCode(err.Code), err.Error)
	}

	fqoid := req.Oid
	logger.Info("GetParam", "slot", slot, "fqoid", fqoid)

	transportContext := s.transport.retrieveMetadataFromContext(ctx)
	param, result := s.transport.runtime.InvokeGetParamHandler(slot, fqoid, transportContext)
	if result.IsError() {
		logger.Error("GetParam handler error", "slot", slot, "fqoid", fqoid, "error", result.Error)
		return nil, status.Error(ToGRPCCode(result.Code), result.Error)
	}
	if param.Proto == nil {
		logger.Error("GetParam returned nil param", "slot", slot, "fqoid", fqoid)
		return nil, status.Error(codes.Internal, "param returned nil")
	}

	return &protos.DeviceComponent_ComponentParam{
		Oid:   fqoid,
		Param: param.Proto,
	}, nil
}

// ParamInfoRequest streams parameter information for the given slot and OID prefix.
func (s *catenaService) ParamInfoRequest(req *protos.ParamInfoRequestPayload, stream grpc.ServerStreamingServer[protos.ParamInfoResponse]) error {
	slot, err := catena.ValidateSlot(req.GetSlot())
	if err.Code != catena.StatusCodeOk {
		return status.Error(ToGRPCCode(err.Code), err.Error)
	}

	oidPrefix := req.GetOidPrefix()
	recursive := req.GetRecursive()
	logger.Info("ParamInfoRequest", "slot", slot, "oid_prefix", oidPrefix, "recursive", recursive)

	transportContext := s.transport.retrieveMetadataFromContext(stream.Context())

	adapter := &grpcStream[st2138.ParamInfo]{ss: stream}
	result := s.transport.runtime.InvokeParamInfoHandler(slot, oidPrefix, recursive, adapter, transportContext)
	if result.IsError() {
		logger.Error("ParamInfoRequest handler error", "slot", slot, "oid_prefix", oidPrefix, "error", result.Error)
		return status.Error(ToGRPCCode(result.Code), result.Error)
	}
	return nil
}

// UpdateSubscriptions handles parameter subscription updates
func (s *catenaService) UpdateSubscriptions(req *protos.UpdateSubscriptionsPayload, stream grpc.ServerStreamingServer[protos.DeviceComponent_ComponentParam]) error {
	// This would need additional handler support in BaseServer for subscriptions
	return status.Error(codes.Unimplemented, "UpdateSubscriptions not implemented")
}

// Connect establishes a streaming connection for push updates
func (s *catenaService) Connect(req *protos.ConnectPayload, stream grpc.ServerStreamingServer[protos.PushUpdates]) error {
	// Register this connection with the runtime using this transport as owner.
	// Owner association allows targeted cleanup (ShutdownTransportConnections(owner))
	// so one transport can shut down without impacting streams owned by others.
	transportContext := s.transport.retrieveMetadataFromContext(stream.Context())
	conn, err := s.transport.runtime.RegisterTransportConnection(s.transport, transportContext)
	if err.Code != catena.StatusCodeOk {
		logger.Error("gRPC connection rejected", "error", err.Error)
		return status.Error(ToGRPCCode(err.Code), err.Error)
	}
	defer s.transport.runtime.DeregisterConnection(conn.ID)

	logger.Info("gRPC Connect started", "connID", conn.ID)

	// Listen for updates and client disconnect
	ctx := stream.Context()
	for {
		select {
		case <-ctx.Done():
			logger.Info("gRPC client disconnected", "connID", conn.ID)
			return ctx.Err()
		case <-conn.Done:
			logger.Info("gRPC connection shut down by server", "connID", conn.ID)
			return status.Error(codes.Unavailable, "server shutting down")
		case update := <-conn.Updates:
			if err := stream.Send(update); err != nil {
				logger.Error("failed to send update", "connID", conn.ID, "error", err)
				return err
			}
		}
	}
}

// ToGRPCCode converts a transport-neutral StatusCode to a gRPC codes.Code.
// Catena StatusCode values 0–16 align 1:1 with google.golang.org/grpc/codes
// per ST 2138-11 §6.2 Table 2; any other input maps to codes.Unknown.
func ToGRPCCode(s catena.StatusCode) codes.Code {
	switch s {
	case catena.StatusCodeOk:
		return codes.OK
	case catena.StatusCodeCancelled:
		return codes.Canceled
	case catena.StatusCodeUnknown:
		return codes.Unknown
	case catena.StatusCodeInvalidArgument:
		return codes.InvalidArgument
	case catena.StatusCodeDeadlineExceeded:
		return codes.DeadlineExceeded
	case catena.StatusCodeNotFound:
		return codes.NotFound
	case catena.StatusCodeAlreadyExists:
		return codes.AlreadyExists
	case catena.StatusCodePermissionDenied:
		return codes.PermissionDenied
	case catena.StatusCodeResourceExhausted:
		return codes.ResourceExhausted
	case catena.StatusCodeFailedPrecondition:
		return codes.FailedPrecondition
	case catena.StatusCodeAborted:
		return codes.Aborted
	case catena.StatusCodeOutOfRange:
		return codes.OutOfRange
	case catena.StatusCodeUnimplemented:
		return codes.Unimplemented
	case catena.StatusCodeInternal:
		return codes.Internal
	case catena.StatusCodeUnavailable:
		return codes.Unavailable
	case catena.StatusCodeDataLoss:
		return codes.DataLoss
	case catena.StatusCodeUnauthenticated:
		return codes.Unauthenticated
	default:
		return codes.Unknown
	}
}

// AddLanguage adds a language pack (optional capability)
func (s *catenaService) AddLanguage(ctx context.Context, req *protos.AddLanguagePayload) (*protos.Empty, error) {
	slot, err := catena.ValidateSlot(req.Slot)
	if err.Code != catena.StatusCodeOk {
		return nil, status.Error(ToGRPCCode(err.Code), err.Error)
	}

	language := req.Language
	logger.Info("AddLanguage", "slot", slot, "language", language)

	// Language and pack presence are validated by the server layer so the rule
	// lives in one place; pass the proto straight through.
	pack := catena.LanguagePack{Proto: req.LanguagePack}

	transportContext := s.transport.retrieveMetadataFromContext(ctx)
	result := s.transport.runtime.InvokeCreateLanguagePackHandler(slot, language, pack, transportContext)
	if result.IsError() {
		logger.Error("AddLanguage handler error", "slot", slot, "language", language, "error", result.Error)
		return nil, status.Error(ToGRPCCode(result.Code), result.Error)
	}

	return &protos.Empty{}, nil
}

// LanguagePackRequest returns a language pack
func (s *catenaService) LanguagePackRequest(ctx context.Context, req *protos.LanguagePackRequestPayload) (*protos.DeviceComponent_ComponentLanguagePack, error) {
	slot, err := catena.ValidateSlot(req.Slot)
	if err.Code != catena.StatusCodeOk {
		return nil, status.Error(ToGRPCCode(err.Code), err.Error)
	}

	language := req.Language
	logger.Info("LanguagePackRequest", "slot", slot, "language", language)

	transportContext := s.transport.retrieveMetadataFromContext(ctx)
	pack, result := s.transport.runtime.InvokeReadLanguagePackHandler(slot, language, transportContext)
	if result.IsError() {
		logger.Error("LanguagePackRequest handler error", "slot", slot, "language", language, "error", result.Error)
		return nil, status.Error(ToGRPCCode(result.Code), result.Error)
	}

	// A nil proto on an OK result is an internal contract violation by the
	// handler, not success; surface it as an internal error rather than
	// returning an empty pack (mirrors REST GET and GetParam).
	if pack.Proto == nil {
		logger.Error("language pack handler returned OK with nil proto", "slot", slot, "language", language)
		return nil, status.Error(codes.Internal, "language pack response was empty")
	}

	// The handler wraps the inner pack (name + words); the gRPC response is the
	// outer component that also carries the language tag.
	return &protos.DeviceComponent_ComponentLanguagePack{
		Language:     language,
		LanguagePack: pack.Proto,
	}, nil
}

// ListLanguages returns the language codes supported by the device model at a slot.
func (s *catenaService) ListLanguages(ctx context.Context, req *protos.Slot) (*protos.LanguageList, error) {
	slot, err := catena.ValidateSlot(req.GetSlot())
	if err.Code != catena.StatusCodeOk {
		return nil, status.Error(ToGRPCCode(err.Code), err.Error)
	}

	logger.Info("ListLanguages", "slot", slot)

	transportContext := s.transport.retrieveMetadataFromContext(ctx)
	languages, result := s.transport.runtime.InvokeListLanguagesHandler(slot, transportContext)
	if result.IsError() {
		logger.Error("ListLanguages handler error", "slot", slot, "error", result.Error)
		return nil, status.Error(ToGRPCCode(result.Code), result.Error)
	}

	return &protos.LanguageList{Languages: languages}, nil
}

// RefreshToken refreshes an authentication token
func (s *catenaService) RefreshToken(ctx context.Context, req *protos.RefreshTokenPayload) (*protos.ConnectionStatus, error) {
	return nil, status.Error(codes.Unimplemented, "RefreshToken not implemented")
}

// RevokeAccess revokes access for a token
func (s *catenaService) RevokeAccess(ctx context.Context, req *protos.RevokeAccessPayload) (*protos.RevocationResponse, error) {
	return nil, status.Error(codes.Unimplemented, "RevokeAccess not implemented")
}
