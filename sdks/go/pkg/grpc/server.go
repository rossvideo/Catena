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
 * @brief gRPC server for the Catena SDK.
 * @file server.go
 * @copyright Copyright © 2026 Ross Video Ltd
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @date 2026-02-25
 */

package grpc

import (
	"context"
	"fmt"
	"net"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/reflection"
	"google.golang.org/grpc/status"

	"github.com/rossvideo/catena/build/go/protos"
	"github.com/rossvideo/catena/sdks/go/pkg/catena"
	"github.com/rossvideo/catena/sdks/go/pkg/internal"
	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

// Server provides gRPC API endpoints for Catena devices
type Server struct {
	*internal.BaseServer
	protos.UnimplementedCatenaServiceServer
	grpcServer *grpc.Server
}

// unaryInterceptor logs all incoming unary RPC calls
func unaryInterceptor(ctx context.Context, req interface{}, info *grpc.UnaryServerInfo, handler grpc.UnaryHandler) (interface{}, error) {
	logger.Debug("gRPC unary call received", "method", info.FullMethod)
	resp, err := handler(ctx, req)
	if err != nil {
		logger.Error("gRPC unary call error", "method", info.FullMethod, "error", err)
	}
	return resp, err
}

// streamInterceptor logs all incoming streaming RPC calls
func streamInterceptor(srv interface{}, ss grpc.ServerStream, info *grpc.StreamServerInfo, handler grpc.StreamHandler) error {
	logger.Debug("gRPC stream call received", "method", info.FullMethod, "isClientStream", info.IsClientStream, "isServerStream", info.IsServerStream)
	err := handler(srv, ss)
	if err != nil {
		logger.Error("gRPC stream call error", "method", info.FullMethod, "error", err)
	}
	return err
}

// NewServer creates a new gRPC server for the given device slots
func NewServer(slots []int) *Server {
	// Create gRPC server with interceptors for logging
	s := &Server{
		BaseServer: internal.NewBaseServer(slots, 0),
		grpcServer: grpc.NewServer(
			grpc.UnaryInterceptor(unaryInterceptor),
			grpc.StreamInterceptor(streamInterceptor),
		),
	}

	// Register this server with the gRPC server
	protos.RegisterCatenaServiceServer(s.grpcServer, s)

	// Register reflection service for grpcurl, grpc_cli, and other tools
	reflection.Register(s.grpcServer)

	logger.Info("gRPC server created", "slots", slots)

	return s
}

// Start starts the gRPC server on the specified port
func (s *Server) Start(port int) error {
	addr := fmt.Sprintf(":%d", port)
	listener, err := net.Listen("tcp", addr)
	if err != nil {
		logger.Error("Failed to create listener", "address", addr, "error", err)
		return fmt.Errorf("failed to listen on %s: %w", addr, err)
	}

	logger.Info("Starting gRPC server", "address", addr, "slots", s.BaseServer.Slots)
	logger.Info("Server listening and ready to accept connections")

	if err := s.grpcServer.Serve(listener); err != nil {
		logger.Error("gRPC server failed", "error", err)
		return err
	}

	return nil
}

// GetPopulatedSlots returns the list of populated slots
func (s *Server) GetPopulatedSlots(ctx context.Context, req *protos.Empty) (*protos.SlotList, error) {
	logger.Info("GetPopulatedSlots")
	slots := make([]uint32, len(s.BaseServer.Slots))
	for i, slot := range s.BaseServer.Slots {
		slots[i] = uint32(slot)
	}
	return &protos.SlotList{Slots: slots}, nil
}

// DeviceRequest streams the device information for a slot
func (s *Server) DeviceRequest(req *protos.DeviceRequestPayload, stream grpc.ServerStreamingServer[protos.DeviceComponent]) error {
	slot := int(req.Slot)
	logger.Info("DeviceRequest", "slot", slot)

	handler := s.BaseServer.GetDeviceHandlers[slot]
	if handler == nil {
		logger.Error("DeviceRequest slot not found", "slot", slot)
		return status.Error(codes.NotFound, "slot not found")
	}

	device, res := handler()
	if res.Error != "" {
		logger.Error("DeviceRequest handler error", "slot", slot, "error", res.Error)
		return status.Error(res.Code.ToGRPCCode(), res.Error)
	}

	protoDevice := device.GetProtoDevice()
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
func (s *Server) GetValue(ctx context.Context, req *protos.GetValuePayload) (*protos.Value, error) {
	slot := int(req.Slot)
	fqoid := req.Oid
	logger.Info("GetValue", "slot", slot, "fqoid", fqoid)

	handler := s.BaseServer.LookupGetValueHandler(slot)
	value, result := handler(slot, fqoid)
	if result.Error != "" {
		logger.Error("GetValue handler error", "slot", slot, "fqoid", fqoid, "error", result.Error)
		return nil, status.Error(result.Code.ToGRPCCode(), result.Error)
	}

	return value.Value, nil
}

// SetValue sets the value of a parameter
func (s *Server) SetValue(ctx context.Context, req *protos.SingleSetValuePayload) (*protos.Empty, error) {
	slot := int(req.Slot)

	if req.Value == nil {
		logger.Error("SetValue nil value payload", "slot", slot)
		return nil, status.Error(codes.InvalidArgument, "value payload is nil")
	}

	fqoid := req.Value.Oid
	logger.Info("SetValue", "slot", slot, "fqoid", fqoid)

	// Convert proto value to native Go type
	nativeValue, err := catena.FromProto(req.Value.Value)
	if err != nil {
		logger.Error("SetValue failed to convert proto value", "slot", slot, "fqoid", fqoid, "error", err)
		return nil, status.Error(codes.InvalidArgument, fmt.Sprintf("invalid value: %v", err))
	}

	handler := s.BaseServer.LookupSetValueHandler(slot)
	result := handler(nativeValue, slot, fqoid)
	if result.Error != "" {
		logger.Error("SetValue handler error", "slot", slot, "fqoid", fqoid, "error", result.Error)
		return nil, status.Error(result.Code.ToGRPCCode(), result.Error)
	}

	return &protos.Empty{}, nil
}

// MultiSetValue sets multiple parameter values
func (s *Server) MultiSetValue(ctx context.Context, req *protos.MultiSetValuePayload) (*protos.Empty, error) {
	slot := int(req.Slot)
	logger.Info("MultiSetValue", "slot", slot, "count", len(req.Values))

	for _, setValue := range req.Values {
		fqoid := setValue.Oid

		nativeValue, err := catena.FromProto(setValue.Value)
		if err != nil {
			logger.Error("MultiSetValue failed to convert proto value", "slot", slot, "fqoid", fqoid, "error", err)
			return nil, status.Error(codes.InvalidArgument, fmt.Sprintf("invalid value for %s: %v", fqoid, err))
		}

		handler := s.BaseServer.LookupSetValueHandler(slot)
		result := handler(nativeValue, slot, fqoid)
		if result.Error != "" {
			logger.Error("MultiSetValue handler error", "slot", slot, "fqoid", fqoid, "error", result.Error)
			return nil, status.Error(result.Code.ToGRPCCode(), result.Error)
		}
	}

	return &protos.Empty{}, nil
}

// ExternalObjectRequest streams external object (asset) data
func (s *Server) ExternalObjectRequest(req *protos.ExternalObjectRequestPayload, stream grpc.ServerStreamingServer[protos.ExternalObjectPayload]) error {
	slot := int(req.Slot)
	fqoid := req.Oid
	logger.Info("ExternalObjectRequest", "slot", slot, "fqoid", fqoid)

	handler := s.BaseServer.LookupGetAssetHandler(slot)
	asset, result := handler(slot, fqoid)
	if result.Error != "" {
		logger.Error("ExternalObjectRequest handler error", "slot", slot, "fqoid", fqoid, "error", result.Error)
		return status.Error(result.Code.ToGRPCCode(), result.Error)
	}

	protoAsset := asset.GetProtoAsset()
	if protoAsset == nil {
		logger.Error("ExternalObjectRequest asset returned nil", "slot", slot, "fqoid", fqoid)
		return status.Error(codes.Internal, "asset returned nil")
	}

	return stream.Send(protoAsset)
}

// ExecuteCommand executes a command and streams the response
func (s *Server) ExecuteCommand(req *protos.ExecuteCommandPayload, stream grpc.ServerStreamingServer[protos.CommandResponse]) error {
	slot := int(req.Slot)
	commandFqoid := req.Oid
	logger.Info("ExecuteCommand", "slot", slot, "command", commandFqoid)

	var payload any
	if req.Value != nil {
		var err error
		payload, err = catena.FromProto(req.Value)
		if err != nil {
			logger.Error("ExecuteCommand failed to convert payload", "slot", slot, "command", commandFqoid, "error", err)
			return status.Error(codes.InvalidArgument, fmt.Sprintf("invalid command payload: %v", err))
		}
	}

	handler := s.BaseServer.LookupExecuteCommandHandler(slot)
	value, result := handler(slot, commandFqoid, payload)
	if result.Error != "" {
		logger.Error("ExecuteCommand handler error", "slot", slot, "command", commandFqoid, "error", result.Error)
		return status.Error(result.Code.ToGRPCCode(), result.Error)
	}

	response := &protos.CommandResponse{}
	if value.Value != nil {
		response.Kind = &protos.CommandResponse_Response{
			Response: value.Value,
		}
	} else {
		response.Kind = &protos.CommandResponse_NoResponse{
			NoResponse: &protos.Empty{},
		}
	}

	return stream.Send(response)
}

// GetParam returns a single parameter's metadata
func (s *Server) GetParam(ctx context.Context, req *protos.GetParamPayload) (*protos.DeviceComponent_ComponentParam, error) {
	// This would need additional handler support in BaseServer for param metadata
	return nil, status.Error(codes.Unimplemented, "GetParam not implemented")
}

// ParamInfoRequest streams parameter information
func (s *Server) ParamInfoRequest(req *protos.ParamInfoRequestPayload, stream grpc.ServerStreamingServer[protos.ParamInfoResponse]) error {
	// This would need additional handler support in BaseServer for param info
	return status.Error(codes.Unimplemented, "ParamInfoRequest not implemented")
}

// UpdateSubscriptions handles parameter subscription updates
func (s *Server) UpdateSubscriptions(req *protos.UpdateSubscriptionsPayload, stream grpc.ServerStreamingServer[protos.DeviceComponent_ComponentParam]) error {
	// This would need additional handler support in BaseServer for subscriptions
	return status.Error(codes.Unimplemented, "UpdateSubscriptions not implemented")
}

// Connect establishes a streaming connection for push updates
func (s *Server) Connect(req *protos.ConnectPayload, stream grpc.ServerStreamingServer[protos.PushUpdates]) error {
	// Register this connection in the connectionQueue
	connID, conn := s.RegisterConnection()
	if connID < 0 {
		logger.Error("gRPC connection rejected - server shutting down")
		return status.Error(codes.Unavailable, "server shutting down")
	}
	defer s.DeregisterConnection(connID)

	logger.Info("gRPC Connect started", "connID", connID)

	// Send initial populated slots
	slots := make([]uint32, len(s.BaseServer.Slots))
	for i, slot := range s.BaseServer.Slots {
		slots[i] = uint32(slot)
	}

	initialUpdate := &protos.PushUpdates{
		Kind: &protos.PushUpdates_SlotsAdded{
			SlotsAdded: &protos.SlotList{
				Slots: slots,
			},
		},
	}

	if err := stream.Send(initialUpdate); err != nil {
		logger.Error("failed to send initial update", "connID", connID, "error", err)
		return status.Error(codes.Internal, "failed to send initial update")
	}

	// Listen for updates and client disconnect
	ctx := stream.Context()
	for {
		select {
		case <-ctx.Done():
			logger.Info("gRPC client disconnected", "connID", connID)
			return ctx.Err()
		case <-conn.Done:
			logger.Info("gRPC connection shut down by server", "connID", connID)
			return status.Error(codes.Unavailable, "server shutting down")
		case update := <-conn.Updates:
			if err := stream.Send(update); err != nil {
				logger.Error("failed to send update", "connID", connID, "error", err)
				return err
			}
		}
	}
}

// Shutdown gracefully shuts down the gRPC server and all streaming connections
func (s *Server) Shutdown() {
	logger.Info("Shutting down gRPC server")
	s.ShutdownConnections()
	s.grpcServer.GracefulStop()
}

// AddLanguage adds a language pack (optional capability)
func (s *Server) AddLanguage(ctx context.Context, req *protos.AddLanguagePayload) (*protos.Empty, error) {
	return nil, status.Error(codes.Unimplemented, "AddLanguage not implemented")
}

// LanguagePackRequest returns a language pack
func (s *Server) LanguagePackRequest(ctx context.Context, req *protos.LanguagePackRequestPayload) (*protos.DeviceComponent_ComponentLanguagePack, error) {
	return nil, status.Error(codes.Unimplemented, "LanguagePackRequest not implemented")
}

// ListLanguages returns the list of available languages
func (s *Server) ListLanguages(ctx context.Context, req *protos.Slot) (*protos.LanguageList, error) {
	return nil, status.Error(codes.Unimplemented, "ListLanguages not implemented")
}

// RefreshToken refreshes an authentication token
func (s *Server) RefreshToken(ctx context.Context, req *protos.RefreshTokenPayload) (*protos.ConnectionStatus, error) {
	return nil, status.Error(codes.Unimplemented, "RefreshToken not implemented")
}

// RevokeAccess revokes access for a token
func (s *Server) RevokeAccess(ctx context.Context, req *protos.RevokeAccessPayload) (*protos.RevocationResponse, error) {
	return nil, status.Error(codes.Unimplemented, "RevokeAccess not implemented")
}
