package main

import (
	"fmt"

	"github.com/rossvideo/catena/sdks/go/pkg/logger"
)

const endpointNameWidth = 26

func logEndpointLine(name, example string) {
	logger.Info(fmt.Sprintf("  %-*s %s", endpointNameWidth, name+":", example))
}

func logGrpcEndpointGuide(host, sampleAsset string) {
	assetOID := sampleAsset
	if assetOID == "" {
		assetOID = "{oid}"
	}

	logger.Info("Use grpcurl or a gRPC client to interact with the server (reflection enabled):")
	logEndpointLine("List services", fmt.Sprintf("grpcurl -plaintext %s list", host))
	logger.Info("")
	logger.Info("Catena API:")
	logEndpointLine("GetPopulatedSlots", fmt.Sprintf("grpcurl -plaintext -d '{}' %s st2138.CatenaService/GetPopulatedSlots", host))
	logEndpointLine("DeviceRequest", fmt.Sprintf("grpcurl -plaintext -d '{\"slot\":0}' %s st2138.CatenaService/DeviceRequest", host))
	logEndpointLine("GetValue", fmt.Sprintf("grpcurl -plaintext -d '{\"slot\":0,\"oid\":\"counter\"}' %s st2138.CatenaService/GetValue", host))
	logEndpointLine("GetParam", fmt.Sprintf("grpcurl -plaintext -d '{\"slot\":0,\"oid\":\"/counter\"}' %s st2138.CatenaService/GetParam", host))
	logEndpointLine("SetValue", fmt.Sprintf("grpcurl -plaintext -d '{\"slot\":0,\"value\":{\"oid\":\"counter\",\"value\":{\"int32_value\":5}}}' %s st2138.CatenaService/SetValue", host))
	logEndpointLine("MultiSetValue", fmt.Sprintf("grpcurl -plaintext -d '{\"slot\":0,\"values\":[{\"oid\":\"counter\",\"value\":{\"int32_value\":5}}]}' %s st2138.CatenaService/MultiSetValue", host))
	logEndpointLine("ExternalObjectRequest", fmt.Sprintf("grpcurl -plaintext -d '{\"slot\":0,\"oid\":\"%s\"}' %s st2138.CatenaService/ExternalObjectRequest", assetOID, host))
	logEndpointLine("ExecuteCommand", fmt.Sprintf("grpcurl -plaintext -d '{\"slot\":0,\"oid\":\"start\"}' %s st2138.CatenaService/ExecuteCommand", host))
	logEndpointLine("ParamInfoRequest (all)", fmt.Sprintf("grpcurl -plaintext -d '{\"slot\":0,\"oid_prefix\":\"\",\"recursive\":true}' %s st2138.CatenaService/ParamInfoRequest", host))
	logEndpointLine("ParamInfoRequest (prefix)", fmt.Sprintf("grpcurl -plaintext -d '{\"slot\":2,\"oid_prefix\":\"struct_example\",\"recursive\":true}' %s st2138.CatenaService/ParamInfoRequest", host))
	logEndpointLine("Connect", fmt.Sprintf("grpcurl -plaintext -d '{}' %s st2138.CatenaService/Connect", host))
}

func logRestEndpointGuide(sampleAsset string) {
	assetOID := sampleAsset
	if assetOID == "" {
		assetOID = "{oid}"
	}

	logger.Info("Catena API:")
	logEndpointLine("GetPopulatedSlots", "GET  /st2138-api/v1/devices")
	logEndpointLine("Health", "GET  /st2138-api/v1/health")
	logEndpointLine("DeviceRequest", "GET  /st2138-api/v1/{slot}")
	logEndpointLine("GetValue", "GET  /st2138-api/v1/{slot}/value/{oid}")
	logEndpointLine("SetValue", "PUT  /st2138-api/v1/{slot}/value/{oid}")
	logEndpointLine("ExternalObjectRequest", fmt.Sprintf("GET  /st2138-api/v1/{slot}/asset/%s", assetOID))
	logEndpointLine("ExternalObjectRequest (GZIP)", fmt.Sprintf("GET  /st2138-api/v1/{slot}/asset/%s?compression=GZIP", assetOID))
	logEndpointLine("ExternalObjectRequest (DEFLATE)", fmt.Sprintf("GET  /st2138-api/v1/{slot}/asset/%s?compression=DEFLATE", assetOID))
	logEndpointLine("ExecuteCommand", "POST /st2138-api/v1/{slot}/command/{oid}")
	logEndpointLine("ParamInfoRequest (unary)", "GET  /st2138-api/v1/{slot}/param-info/{oid}")
	logEndpointLine("ParamInfoRequest (all)", "GET  /st2138-api/v1/{slot}/param-info/stream")
	logEndpointLine("ParamInfoRequest (recursive)", "GET  /st2138-api/v1/{slot}/param-info/{oid}/stream?recursive")
	logEndpointLine("Connect", "GET  /st2138-api/v1/connect")
	logger.Info("")
	logger.Info("Demo (non-Catena):")
	logEndpointLine("Web UI", "GET  /")
	logEndpointLine("Web UI styles", "GET  /styles.css")
	logEndpointLine("Web UI script", "GET  /script.js")
	logEndpointLine("Asset index", "GET  /assets-list")
}
