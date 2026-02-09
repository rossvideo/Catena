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
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file ConnectionProps.h
 * @brief Lightweight HTTP server for serving connection props endpoints
 * @author Christian Twarog (christian.twarog@rossvideo.com)
 * @copyright Copyright © 2026 Ross Video Ltd
 */

 #pragma once

 #include <boost/asio.hpp>
 #include <string>
 #include <thread>
 #include <atomic>
 #include <memory>
 
 using boost::asio::ip::tcp;
 
 namespace catena {
 namespace common {

/**
 * @brief The default port for the connection props server.
 */
const uint16_t DEFAULT_CONNECTION_PROPS_PORT = 80;

/**
 * @brief Configuration structure for connection properties
 */
struct ConnectionPropsConfig {
    std::string protocol = "grpc";           // "grpc" or "rest"
    std::string address = "localhost";       // Hostname or IP address
    uint16_t service_port = 6254;            // Port of the actual service
    std::string node_id = "";                // Unique node identifier
    std::string node_name = "";              // Human-readable node name
    uint32_t refresh_interval = 30000;       // Refresh interval in milliseconds
    bool use_tls = false;                    // Whether to use HTTPS/TLS
};
 
 /**
  * @brief Lightweight HTTP server for serving connection props information
  * 
  * This server listens on a configurable port (default DEFAULT_CONNECTION_PROPS_PORT) and serves a single
  * endpoint (/connect/connection-props.xml) with connection properties for
  * connection props. It runs in a background thread and can be started/stopped
  * cleanly. It generates XML content from configuration parameters.
  * 
  * Example usage:
  * @code
  * ConnectionPropsConfig config;
  * config.protocol = "grpc";
  * config.address = "192.168.1.100";
  * config.service_port = 6254;
  * config.node_id = "1234567890";
  * config.node_name = "My Node";
  * config.refresh_interval = 30000;
  * config.use_tls = false;
  * ConnectionProps server(config, "/connect/connection-props.xml", 8080);
  * server.start();
  * // ... run your main service ...
  * server.stop();
  * @endcode
  */
 class ConnectionProps {
 public:
     /**
      * @brief Construct a new Connection Props Server with configuration
      * 
      * @param config Configuration for generating connection properties
      * @param endpoint The endpoint path (default "/connect/connection-props.xml")
      * @param port The port to listen on (default DEFAULT_CONNECTION_PROPS_PORT)
      */
     ConnectionProps(
         const ConnectionPropsConfig& config,
         const std::string& endpoint = "/connect/connection-props.xml",
         uint16_t port = DEFAULT_CONNECTION_PROPS_PORT);

    /**
    * @brief Destructor - stops the server if running
    */
    ~ConnectionProps();

    // Disable copy and move
    ConnectionProps(const ConnectionProps&) = delete;
    ConnectionProps& operator=(const ConnectionProps&) = delete;
    ConnectionProps(ConnectionProps&&) = delete;
    ConnectionProps& operator=(ConnectionProps&&) = delete;

    /**
    * @brief Start the connection props server in a background thread
    * @return true if started successfully, false otherwise
    */
    bool start();

    /**
    * @brief Stop the connection props server
    */
    void stop();

    /**
    * @brief Check if the server is running
    * @return true if running, false otherwise
    */
    bool isRunning() const { return running_; }

     /**
      * @brief Update the response content
      * @param content The new XML content to serve
      * @deprecated Use updateConfig() instead
      */
     void updateContent(const std::string& content);

     /**
      * @brief Update the configuration and regenerate XML
      * @param config The new configuration
      */
     void updateConfig(const ConnectionPropsConfig& config);

    /**
    * @brief Get the current port
    * @return The port number
    */
    uint16_t getPort() const { return port_; }

    /**
    * @brief Get the endpoint path
    * @return The endpoint path
    */
    const std::string& getEndpoint() const { return endpoint_; }

private:
    /**
    * @brief Main server loop running in background thread
    */
    void run();

    /**
    * @brief Handle a single client connection
    * @param socket The client socket
    */
    void handleRequest(tcp::socket socket);

     /**
      * @brief Generate HTTP response
      * @param path The requested path
      * @return HTTP response string
      */
     std::string generateResponse(const std::string& path);

     /**
      * @brief Generate XML content from configuration
      * @param config The configuration to generate XML from
      * @return XML string
      */
     static std::string generateXml(const ConnectionPropsConfig& config);

    /**
    * @brief Port to listen on
    */
    uint16_t port_;

    /**
    * @brief Endpoint path (e.g., "/connect/connection-props.xml")
    */
    std::string endpoint_;

     /**
      * @brief Content to serve at the endpoint
      */
     std::string response_content_;

     /**
      * @brief Mutex to protect response_content_ and config_
      */
     mutable std::mutex content_mutex_;

     /**
      * @brief Configuration for generating connection properties
      */
     ConnectionPropsConfig config_;

    /**
    * @brief IO context for async operations
    */
    std::unique_ptr<boost::asio::io_context> io_context_;

    /**
    * @brief Acceptor for incoming connections
    */
    std::unique_ptr<tcp::acceptor> acceptor_;

    /**
    * @brief Background thread running the server
    */
    std::unique_ptr<std::thread> server_thread_;

    /**
    * @brief Flag indicating if server is running
    */
    std::atomic<bool> running_;
};

} // namespace common
} // namespace catena
