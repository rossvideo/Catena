#pragma once

/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//

/**
 * @brief Communicates failure status using gRPC's codes
 * @file Status.h
 * @copyright Copyright © 2023 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

#include <sstream>
#include <stdexcept>

namespace catena {

/**
 * @brief Status Code - reproduced from google's grpc project.
 * 
 * The license for this code is Apache 2.0 which permits redistribution
 * and modification. The original code is available at: https://github.com/grpc/grpc
 * 
 * The governing license is available here: http://www.apache.org/licenses/LICENSE-2.0
 * 
 * The intent here is to re-use the grpc status codes without creating a 
 * dependency on the grpc project. The risk of the values of these codes 
 * changing is low, but if it does happen, we can always update this file.
 * 
 */
enum StatusCode {
  /// Not an error; returned on success.
  OK = 0,

  /// The operation was cancelled (typically by the caller).
  CANCELLED = 1,

  /// Unknown error. An example of where this error may be returned is if a
  /// Status value received from another address space belongs to an error-space
  /// that is not known in this address space. Also errors raised by APIs that
  /// do not return enough error information may be converted to this error.
  UNKNOWN = 2,

  /// Client specified an invalid argument. Note that this differs from
  /// FAILED_PRECONDITION. INVALID_ARGUMENT indicates arguments that are
  /// problematic regardless of the state of the system (e.g., a malformed file
  /// name).
  INVALID_ARGUMENT = 3,

  /// Deadline expired before operation could complete. For operations that
  /// change the state of the system, this error may be returned even if the
  /// operation has completed successfully. For example, a successful response
  /// from a server could have been delayed long enough for the deadline to
  /// expire.
  DEADLINE_EXCEEDED = 4,

  /// Some requested entity (e.g., file or directory) was not found.
  NOT_FOUND = 5,

  /// Some entity that we attempted to create (e.g., file or directory) already
  /// exists.
  ALREADY_EXISTS = 6,

  /// The caller does not have permission to execute the specified operation.
  /// PERMISSION_DENIED must not be used for rejections caused by exhausting
  /// some resource (use RESOURCE_EXHAUSTED instead for those errors).
  /// PERMISSION_DENIED must not be used if the caller can not be identified
  /// (use UNAUTHENTICATED instead for those errors).
  PERMISSION_DENIED = 7,

  /// The request does not have valid authentication credentials for the
  /// operation.
  UNAUTHENTICATED = 16,

  /// Some resource has been exhausted, perhaps a per-user quota, or perhaps the
  /// entire file system is out of space.
  RESOURCE_EXHAUSTED = 8,

  /// Operation was rejected because the system is not in a state required for
  /// the operation's execution. For example, directory to be deleted may be
  /// non-empty, an rmdir operation is applied to a non-directory, etc.
  ///
  /// A litmus test that may help a service implementor in deciding
  /// between FAILED_PRECONDITION, ABORTED, and UNAVAILABLE:
  ///  (a) Use UNAVAILABLE if the client can retry just the failing call.
  ///  (b) Use ABORTED if the client should retry at a higher-level
  ///      (e.g., restarting a read-modify-write sequence).
  ///  (c) Use FAILED_PRECONDITION if the client should not retry until
  ///      the system state has been explicitly fixed. E.g., if an "rmdir"
  ///      fails because the directory is non-empty, FAILED_PRECONDITION
  ///      should be returned since the client should not retry unless
  ///      they have first fixed up the directory by deleting files from it.
  ///  (d) Use FAILED_PRECONDITION if the client performs conditional
  ///      REST Get/Update/Delete on a resource and the resource on the
  ///      server does not match the condition. E.g., conflicting
  ///      read-modify-write on the same resource.
  FAILED_PRECONDITION = 9,

  /// The operation was aborted, typically due to a concurrency issue like
  /// sequencer check failures, transaction aborts, etc.
  ///
  /// See litmus test above for deciding between FAILED_PRECONDITION, ABORTED,
  /// and UNAVAILABLE.
  ABORTED = 10,

  /// Operation was attempted past the valid range. E.g., seeking or reading
  /// past end of file.
  ///
  /// Unlike INVALID_ARGUMENT, this error indicates a problem that may be fixed
  /// if the system state changes. For example, a 32-bit file system will
  /// generate INVALID_ARGUMENT if asked to read at an offset that is not in the
  /// range [0,2^32-1], but it will generate OUT_OF_RANGE if asked to read from
  /// an offset past the current file size.
  ///
  /// There is a fair bit of overlap between FAILED_PRECONDITION and
  /// OUT_OF_RANGE. We recommend using OUT_OF_RANGE (the more specific error)
  /// when it applies so that callers who are iterating through a space can
  /// easily look for an OUT_OF_RANGE error to detect when they are done.
  OUT_OF_RANGE = 11,

  /// Operation is not implemented or not supported/enabled in this service.
  UNIMPLEMENTED = 12,

  /// Internal errors. Means some invariants expected by underlying System has
  /// been broken. If you see one of these errors, Something is very broken.
  INTERNAL = 13,

  /// The service is currently unavailable. This is a most likely a transient
  /// condition and may be corrected by retrying with a backoff. Note that it is
  /// not always safe to retry non-idempotent operations.
  ///
  /// \warning Although data MIGHT not have been transmitted when this
  /// status occurs, there is NOT A GUARANTEE that the server has not seen
  /// anything. So in general it is unsafe to retry on this status code
  /// if the call is non-idempotent.
  ///
  /// See litmus test above for deciding between FAILED_PRECONDITION, ABORTED,
  /// and UNAVAILABLE.
  UNAVAILABLE = 14,

  /// Unrecoverable data loss or corruption.
  DATA_LOSS = 15,

  /// Force users to include a default branch:
  DO_NOT_USE = -1
};

/**
 * @brief A std::runtime_error with a gRPC StatusCode. This struct cannot be
 * copied
 */
struct exception_with_status : public std::runtime_error {
    inline explicit exception_with_status(const std::string &why, StatusCode s)
        : std::runtime_error{why}, status{s} {}

    inline exception_with_status(exception_with_status&&) = default;
    inline exception_with_status(const exception_with_status&) = delete;
    inline exception_with_status& operator=(exception_with_status&&) = default;
    inline exception_with_status& operator=(const exception_with_status&) = delete;

    StatusCode status;
};

}  // namespace catena

/**
 * @brief creates a catena::exception_with_status object and throws it.
 * @todo condition whether __FILE__ and __LINE__ are reported on the build type
 * this info should be restricted to dev and dbg builds and not go into 
 * release / production ones.
 * 
 */
#define BAD_STATUS(msg, status)                                                                              \
    do {                                                                                                     \
        static_assert(status != catena::StatusCode::OK, "Only use with error status codes");                   \
        std::stringstream why;                                                                               \
        why << __FILE__ << ':' << __LINE__ << '\n' << msg;                                                   \
        throw catena::exception_with_status(why.str(), status);                                              \
    } while (false)
