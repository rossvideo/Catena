#pragma once

/*
 * Copyright 2024 Ross Video Ltd
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
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
 * @brief Tools for using EnumDecorators.
 * @file Enums.h
 * @author John R. Naylor
 * @copyright Copyright (c) 2022, Ross Video Limited. All Rights Reserved.
 */

// common
#include <patterns/EnumDecorator.h>

// protobuf interface
#include <interface/device.pb.h>

#include <cstdint>


namespace catena {
namespace common {

/**
 * @brief Scopes_e is an enumeration of the different access scopes that a
 * device supports.
 */
enum class Scopes_e : int32_t { kUndefined, kMonitor, kOperate, kConfig, kAdmin };


// Instantiate the EnumDecorator for the Scopes_e and DetailLevel_e enums.
using Scopes = patterns::EnumDecorator<common::Scopes_e>;
using DetailLevel = patterns::EnumDecorator<st2138::Device_DetailLevel>;

} // namespace common

/**
 * @brief Map or access scopes to strings.
 */
template <>
inline const common::Scopes::FwdMap common::Scopes::fwdMap_ = {
  {common::Scopes_e::kUndefined, "undefined"},
  {common::Scopes_e::kMonitor,   "st2138:mon"},
  {common::Scopes_e::kOperate,   "st2138:op"},
  {common::Scopes_e::kConfig,    "st2138:cfg"},
  {common::Scopes_e::kAdmin,     "st2138:adm"}
};

/**
 * @brief Map or detail levels to strings.
 */
template <>
inline const common::DetailLevel::FwdMap common::DetailLevel::fwdMap_ = {
  {st2138::Device_DetailLevel_FULL,          "FULL"},
  {st2138::Device_DetailLevel_SUBSCRIPTIONS, "SUBSCRIPTIONS"},
  {st2138::Device_DetailLevel_MINIMAL,       "MINIMAL"},
  {st2138::Device_DetailLevel_COMMANDS,      "COMMANDS"},
  {st2138::Device_DetailLevel_NONE,          "NONE"},
  {st2138::Device_DetailLevel_UNSET,         "UNSET"}
};

} // namespace catena
