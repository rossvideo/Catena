#pragma once

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
 * @file RestJsonFormatter.h
 * @brief Central proto-to-JSON formatter for the REST connection layer.
 *
 * protobuf's JSON serializer cannot, with a single set of options, both emit a
 * scalar at its proto3 default (e.g. Device.slot == 0) and omit an empty
 * polymorphic branch (e.g. a non-struct Param's empty params map). This
 * formatter follows the Go SDK's "serialize then fix" strategy: it owns the
 * call to protobuf and the error translation, and dispatches per-message-type
 * options plus an optional post-processing pass via a registry keyed by the
 * message Descriptor.
 *
 * The registry is populated once at construction and is read-only at runtime
 * (no locking). Test-only mutators are provided so suites can install or
 * bypass rules in isolation.
 *
 * @copyright Copyright © 2026 Ross Video Ltd
 */

// common
#include <Status.h>
#include <patterns/Singleton.h>

// proto
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

// std
#include <functional>
#include <string>
#include <unordered_map>

namespace catena {
namespace REST {

/**
 * @brief Singleton that converts protobuf messages to st2138-conformant JSON.
 */
class RestJsonFormatter : public catena::patterns::Singleton<RestJsonFormatter> {
  public:
    /**
     * @brief Post-processing function run on the serialized JSON. Receives the
     * originating message (for fields that were omitted in serialization) and
     * the JSON string to fix in place.
     */
    using PostFn = std::function<void(const google::protobuf::Message&, std::string&)>;

    /**
     * @brief A per-message-type rule: serialization options plus an optional
     * post-processing pass.
     */
    struct Rule {
        google::protobuf::util::JsonPrintOptions options{};
        PostFn post{};
    };

    /**
     * @brief Type of the descriptor-keyed rule table.
     */
    using RuleTable = std::unordered_map<const google::protobuf::Descriptor*, Rule>;

    /**
     * @brief Constructs the formatter and registers the default rule table.
     * The Protector argument (inaccessible outside the Singleton base) keeps
     * construction confined to Singleton::getInstance().
     */
    RestJsonFormatter(Protector);

    /**
     * @brief Serializes @p msg to JSON in @p out, applying any registered rule.
     *
     * Proto field names are always preserved (snake_case, as the st2138 spec
     * requires). On serialization failure the internal absl::Status is
     * translated to a catena::exception_with_status carrying
     * StatusCode::INVALID_ARGUMENT; on success the returned status is OK.
     * @param msg The message to serialize.
     * @param out Output string, cleared and overwritten with the JSON.
     * @return OK on success, INVALID_ARGUMENT on serialization failure.
     */
    catena::exception_with_status format(const google::protobuf::Message& msg, std::string& out) const;

    /**
     * @brief Installs or replaces the rule for @p desc. Test/setup use only.
     */
    void setRule(const google::protobuf::Descriptor* desc, Rule rule);

    /**
     * @brief Returns true if a rule is registered for @p desc.
     */
    bool hasRule(const google::protobuf::Descriptor* desc) const;

    /**
     * @brief Returns a copy of the current rule table. Test/setup use only.
     */
    RuleTable snapshot() const;

    /**
     * @brief Replaces the entire rule table. Test/setup use only.
     */
    void restore(RuleTable rules);

  private:
    RuleTable rules_;
};

}  // namespace REST
}  // namespace catena
