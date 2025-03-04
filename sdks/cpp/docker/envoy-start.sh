#!/bin/bash

# Replace placeholders in the template file with environment variables
cat <<EOF > /tmp/envoy/envoy.yaml
# This YAML file is used to configure the Envoy proxy server that verifies JWS
# tokens using the AUTHZ server and routes gRPC requests to the gRPC server
# running in the container.

static_resources:

  # Listening on port 6254 for incoming requests.
  listeners:
  - name: listener_0
    address:
      socket_address:
        address: 0.0.0.0
        port_value: 6254

    filter_chains:
    # Filters out http requests from the port above.
    - filters:
      - name: envoy.filters.network.http_connection_manager
        typed_config:
          "@type": type.googleapis.com/envoy.extensions.filters.network.http_connection_manager.v3.HttpConnectionManager
          stat_prefix: ingress_http
          # Required to log info
          access_log:
          - name: envoy.access_loggers.stdout
            typed_config:
              "@type": type.googleapis.com/envoy.extensions.access_loggers.stream.v3.StdoutAccessLog
          http_filters:

          # First filter verifies the JWS token. If invalid, it cancels the
          # request.
          - name: envoy.filters.http.jwt_authn
            typed_config:
              "@type": type.googleapis.com/envoy.extensions.filters.http.jwt_authn.v3.JwtAuthentication
              providers:
                jwtProvider:
                  issuer: ${AUTHZ_SERVER}/realms/${REALM}
                  remote_jwks:
                    http_uri:
                      uri: ${AUTHZ_SERVER}/realms/${REALM}/protocol/openid-connect/certs
                      cluster: jwks_cluster
                      timeout: 5s
                  forward: true
              rules:
              - match:
                  prefix: "/"
                requires:
                  provider_name: jwtProvider

          # If verified, the second filter routes the request.
          - name: envoy.filters.http.router
            typed_config:
              "@type": type.googleapis.com/envoy.extensions.filters.http.router.v3.Router

          # Routes the valid request to the gRPC_custer.
          route_config:
            name: local_route
            virtual_hosts:
            - name: default
              domains: ["*"]
              routes:
              - match:
                  prefix: "/"
                route:
                  #host_rewrite_literal: www.envoyproxy.io
                  cluster: grpc_cluster
  clusters:
  # Cluster for handling jwks.
  - name: jwks_cluster
    type: STRICT_DNS
    lb_policy: ROUND_ROBIN
    load_assignment:
      cluster_name: jwks_cluster
      endpoints:
      - lb_endpoints:
        - endpoint:
            address:
              socket_address:
                address: ${AUTHZ_SERVER}
                port_value: 443 # JWKS endpoint.
    transport_socket:
      name: envoy.transport_sockets.tls
      typed_config:
        "@type": type.googleapis.com/envoy.extensions.transport_sockets.tls.v3.UpstreamTlsContext
        sni: ${AUTHZ_SERVER}

  # Cluster for handling authenticated gRPC requests.
  # Passes them off to port 50051.
  - name: grpc_cluster
    type: STRICT_DNS
    typed_extension_protocol_options:
      envoy.extensions.upstreams.http.v3.HttpProtocolOptions:
        "@type": type.googleapis.com/envoy.extensions.upstreams.http.v3.HttpProtocolOptions
        explicit_http_config:
          http2_protocol_options: {}
    load_assignment:
      cluster_name: grpc_cluster
      endpoints:
      - lb_endpoints:
        - endpoint:
            address:
              socket_address:
                address: catena-develop-container
                port_value: 50051 # Typical port for gRPC calls.
EOF

echo "Created file"

# Start Envoy with the generated configuration file
envoy -c /tmp/envoy/envoy.yaml