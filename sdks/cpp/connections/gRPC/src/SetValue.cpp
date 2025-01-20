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

// common
#include <Tags.h>

// connections/gRPC
#include <SetValue.h>

// type aliases
using catena::common::ParamTag;
using catena::common::Path;

#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iterator>
#include <filesystem>

// Initializes the object counter for SetValue to 0.
int CatenaServiceImpl::SetValue::objectCounter_ = 0;

/**
 * Constructor which initializes and registers the current SetValue object, 
 * then starts the process.
 */
CatenaServiceImpl::SetValue::SetValue(CatenaServiceImpl *service, Device &dm, bool ok)
    : GenericSetValue(service, dm, ok, objectCounter_++) {
    typeName = "SetValue";
    proceed(service, ok);
}

/**
 * Gets a SetValueRequest from the system and adds it to the
 * MultiSetValuePayload in GenericSetValue.
 */
void CatenaServiceImpl::SetValue::request() {
    catena::SetValuePayload* req = reqs_.add_values();
    service_->RequestSetValue(&context_, req, &responder_, service_->cq_, service_->cq_, this);
}

// Creates a new SetValue to handle requests while processing.
void CatenaServiceImpl::SetValue::create(CatenaServiceImpl *service, Device &dm, bool ok) {
    new SetValue(service, dm, ok);
}
