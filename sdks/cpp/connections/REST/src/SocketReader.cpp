
#include <SocketReader.h>
#include <string_view>
#include <boost/asio/as_tuple.hpp>
using catena::REST::SocketReader;

namespace {
static inline bool iequals_header_name(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    const unsigned char* pa = reinterpret_cast<const unsigned char*>(a.data());
    const unsigned char* pb = reinterpret_cast<const unsigned char*>(b.data());
    const std::size_t n = a.size();

    for (std::size_t i = 0; i < n; ++i) {
        unsigned char ca = pa[i];
        unsigned char cb = pb[i]; // already lowercase

        if (ca == cb) continue; // fast path

        // Fold only 'A'..'Z' to 'a'..'z'
        if (ca >= 'A' && ca <= 'Z') {
            ca = static_cast<unsigned char>(ca | 0x20); // to lowercase
            if (ca == cb) continue;
        }

        // If we get here, they are not equal
        return false;
    }
    return true;
}

/**
 * Result from read coroutine.
 * Regular read will populate the vector,
 * Read until will populate the streambuf
 */
struct ReadResult {
    std::vector<char> vecBuffer;
    std::unique_ptr<boost::asio::streambuf> streamBuffer;
    boost::system::error_code ec;

    // Needed for co_spawn to work
    ReadResult() = default;

    // Used in read coroutines
    ReadResult(std::vector<char> vb, std::unique_ptr<boost::asio::streambuf> sb, boost::system::error_code e)
        : vecBuffer(std::move(vb)), streamBuffer(std::move(sb)), ec(e) {}
};

boost::asio::awaitable<ReadResult> read_with_timeout(std::shared_ptr<tcp::socket> socket, std::size_t bytes, int timeout) {
    using namespace boost::asio;
    using namespace experimental::awaitable_operators;
    std::vector<char> buf(bytes);
    steady_timer timer(socket->get_executor(), std::chrono::milliseconds(timeout));
    

    // Async_read and timer run simultaneously, the other cancels when one finishes
    auto result = co_await (async_read(*socket, buffer(buf), as_tuple(use_awaitable)) ||
        timer.async_wait(as_tuple(use_awaitable))
    );

    // index of 0 = read finished first
    if (result.index() == 0) {
        auto [ec, n] = std::get<0>(result);
        if (!ec) {
            co_return ReadResult(std::move(buf), nullptr, {});
        } else {
            co_return ReadResult({}, nullptr, ec);
        }
    } else {
        co_return ReadResult({}, nullptr, boost::asio::error::timed_out);
    }
}

boost::asio::awaitable<ReadResult> read_until_with_timeout(std::shared_ptr<tcp::socket> socket, int timeout) {
    using namespace boost::asio;
    using namespace experimental::awaitable_operators;
    auto buf = std::make_unique<boost::asio::streambuf>();
    steady_timer timer(socket->get_executor(), std::chrono::milliseconds(timeout));


    // Async_read and timer run simultaneously, the other cancels when one finishes
    auto result = co_await (async_read_until(*socket, *buf, "\r\n\r\n", as_tuple(use_awaitable)) ||
        timer.async_wait(as_tuple(use_awaitable))
    );

    // index of 0 = read finished first
    if (result.index() == 0) {
        auto [ec, n] = std::get<0>(result);
        if (!ec) {
            co_return ReadResult({}, std::move(buf), {});
        } else {
            co_return ReadResult({}, nullptr, ec);
        }
    } else {
        co_return ReadResult({}, nullptr, boost::asio::error::timed_out);
    }
}

static inline bool valid_content_type(std::string_view s, std::string_view contentType) {
    if (s.size() < contentType.size()) return false;
    const unsigned char* ps = reinterpret_cast<const unsigned char*>(s.data());
    const unsigned char* pcontentType = reinterpret_cast<const unsigned char*>(contentType.data());
    const std::size_t n = contentType.size();

    for (std::size_t i = 0; i < n; ++i) {
        unsigned char cs = ps[i];
        unsigned char ccontentType = pcontentType[i]; // already lowercase

        if (cs == ccontentType) continue; // fast path

        // Fold only 'A'..'Z' to 'a'..'z'
        if (cs >= 'A' && cs <= 'Z') {
            cs = static_cast<unsigned char>(cs | 0x20); // to lowercase
            if (cs == ccontentType) continue;
        }

        // If we get here, they are not equal
        return false;
    }
    // Ensure MIME type is exactly contentType
    if (n != s.size() && ps[n] != ';') {
        return false;
    }
    return true;

}
}

void SocketReader::read(std::shared_ptr<tcp::socket>& socket, uint32_t timeout) {
    // Resetting variables.
    method_ = catena::REST::Method_NONE;
    slot_ = 0;
    endpoint_ = "";
    fqoid_ = "";
    stream_ = false;
    origin_ = "";
    detailLevel_ = st2138::Device_DetailLevel_UNSET;
    jwsToken_ = "";
    jsonBody_ = "";
    requestStart_ = DEFAULT_REQUEST_START;
    requestReceived_ = DEFAULT_REQUEST_RECEIVED;

    // Getting request receival time formatted as,
    // <number of milliseconds since start of epoch>
    const auto epoch_time = std::chrono::system_clock::now().time_since_epoch();
    requestReceived_ = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(epoch_time).count());

    bool hasContentType = false; // Used for enforcing Content-Type

    // Reading from the socket.
    auto fut = boost::asio::co_spawn(socket->get_executor(), read_until_with_timeout(socket, timeout), boost::asio::use_future);
    ReadResult result = fut.get();
    if (result.ec == boost::asio::error::timed_out) {
        throw catena::exception_with_status("Timed out", catena::StatusCode::DEADLINE_EXCEEDED);
    } else if (result.ec) {
        throw catena::exception_with_status("Read error", catena::StatusCode::UNKNOWN);
    }
    std::istream header_stream(result.streamBuffer.get());

    // Getting the first line from the stream (URL), splitting, and parsing.
    std::string header;
    std::getline(header_stream, header);
    std::string url, httpVersion;
    std::string methodStr;
    std::istringstream(header) >> methodStr >> url >> httpVersion;

    // Converting method to enum.
    auto& methodMap = RESTMethodMap().getReverseMap();
    if (methodMap.contains(methodStr)) {
        method_ = methodMap.at(methodStr);
    }

    // Parsing url
    url_view u(url);
    try {
        std::vector<std::string> path;
        catena::split(path, u.path(), "/");
        // Checking the url starts with "st2138-api/v1/"
        if (path.size() >= 4 && path[1] == "st2138-api" && path[2] == service_->version()) {
            try {
                slot_ = std::stoi(path[3]);
            } catch(...) {
                endpoint_ = "/" + path[3];
            }
            // If the stream flag was added pop it from the path.
            if (path.back() == "stream") {
                path.pop_back();
                stream_ = true;
            }
            // Lastly getting the endpoint (if not already set) and the fqoid.
            if (path.size() > 4) {
                uint32_t i = 4;
                // First segment after "api/v1/slot" is the endpoint.
                if (endpoint_.empty()) {
                    endpoint_ = "/" + path[i];
                    i++;
                }
                // Everything after the endpoint is the fqoid.
                for (; i < path.size(); i++) {
                    fqoid_ += "/" + path.at(i);
                }
            }
        } else {
            throw catena::exception_with_status("Invalid URL", catena::StatusCode::INVALID_ARGUMENT);
        }
    } catch (...) {
        throw catena::exception_with_status("Invalid URL", catena::StatusCode::INVALID_ARGUMENT);
    }

    // Parsing query parameters.
    for (auto element : u.encoded_params()) {
        fields_[(std::string)element.key] = element.value;
    }
    
    // Looping through headers to retrieve JWS token and json body len.
    std::size_t contentLength = 0;
    while(std::getline(header_stream, header) && header != "\r") {
        // Parse "<name>: <value>" and handle header name case-insensitively
        std::size_t sep = header.find(':');
        if (sep == std::string::npos) {
            continue;
        }
        std::string name = header.substr(0, sep);
        std::string value = header.substr(sep + 1);
        // Trim leading spaces
        while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
            value.erase(0, 1);
        }
        // Trim trailing CR and spaces
        while (!value.empty() && (value.back() == '\r' || value.back() == ' ' || value.back() == '\t')) {
            value.pop_back();
        }

        // Getting jwsToken.
        if (service_->authorizationEnabled() && jwsToken_.empty() && iequals_header_name(name, "authorization")) {
            // Expect "Bearer <token>" (keep scheme check case-sensitive as before)
            static const std::string kBearerPrefix = "Bearer ";
            if (value.find(kBearerPrefix) == 0) {
                jwsToken_ = value.substr(kBearerPrefix.length());
            }
        }
        // Getting origin
        else if (origin_.empty() && iequals_header_name(name, "origin")) {
            origin_ = value;
        }
        // Getting detail level from header
        else if (detailLevel_ == st2138::Device_DetailLevel_UNSET && iequals_header_name(name, "detail-level")) {
            std::string dl = value;
            auto& dlMap = catena::common::DetailLevel().getReverseMap();
            if (dlMap.contains(dl)) {
                detailLevel_ = dlMap.at(dl);
            }
        }
        // Getting time request was sent
        else if (requestStart_ == DEFAULT_REQUEST_START && iequals_header_name(name, "request-start")){
            catena::readTimestamp(value, requestStart_);
        }
        // Getting body content-Length
        else if (contentLength == 0 && iequals_header_name(name, "content-length")) {
            try {
                if (value.empty() || value[0] == '-') {
                    throw catena::exception_with_status("Invalid Content-Length", catena::StatusCode::INVALID_ARGUMENT);
                }
                size_t processed = 0;
                contentLength = stoi(value, &processed);
                if (processed < value.length()) {
                    throw catena::exception_with_status("Invalid Content-Length", catena::StatusCode::INVALID_ARGUMENT);
                }
            } catch(...) {
                throw catena::exception_with_status("Invalid Content-Length", catena::StatusCode::INVALID_ARGUMENT);
            }
        }
        // Checking Content-Type
        else if (iequals_header_name(name, "content-type")) {
            if (!valid_content_type(value, "application/json")) {
                throw catena::exception_with_status("Invalid Content-Type", catena::StatusCode::INVALID_ARGUMENT);
            }
            hasContentType = true;
        }
    }
    // If body exists, we need to handle leftover data and append the rest.
    jsonBody_ = std::string((std::istreambuf_iterator<char>(header_stream)), std::istreambuf_iterator<char>());
    if (contentLength <= 0 && jsonBody_.size() > 0) {
        throw catena::exception_with_status("Incorrect Content-Length: data lost", catena::StatusCode::DATA_LOSS);
    }
    if (contentLength > 0) {
        // All request bodies must be json according to the spec, so we can just check here
        if (!hasContentType) {
            throw catena::exception_with_status("Content-Type missing", catena::StatusCode::INVALID_ARGUMENT);
        }
        if (jsonBody_.size() < contentLength) {
            std::size_t leftover = contentLength - jsonBody_.size();
            std::size_t start = jsonBody_.size();

            auto fut = boost::asio::co_spawn(socket->get_executor(), read_with_timeout(socket, leftover, timeout), boost::asio::use_future);
            ReadResult result = fut.get();
            jsonBody_.append(result.vecBuffer.begin(), result.vecBuffer.end());
            if (result.ec == boost::asio::error::timed_out) {
                throw catena::exception_with_status("Timed out", catena::StatusCode::DEADLINE_EXCEEDED);
            } else if (result.ec) {
                throw catena::exception_with_status("Read error", catena::StatusCode::UNKNOWN);
            }
        } else if (jsonBody_.size() > contentLength) {
            throw catena::exception_with_status("Incorrect Content-Length: data lost", catena::StatusCode::DATA_LOSS);
        }
    }
    // Setting detail level to NONE if not set.
    if (detailLevel_ == st2138::Device_DetailLevel_UNSET) {
        detailLevel_ = st2138::Device_DetailLevel_NONE;
    }
}
