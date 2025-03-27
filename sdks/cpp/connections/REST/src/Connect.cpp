// common
#include <Tags.h>
#include <Enums.h>

#include <interface/device.pb.h>
#include <google/protobuf/util/json_util.h>
#include <utils.h>

#include <Connect.h>

#include "absl/flags/flag.h"

#include <iostream>
#include <regex>

using catena::API;

// Initializes the object counter for Connect to 0.
int API::Connect::objectCounter_ = 0;

// Initializing the shutdown signal for all open connections.
vdk::signal<void()> API::Connect::shutdownSignal_;

API::Connect::Connect(std::string& request, Tcp::socket& socket, Device& dm, catena::common::Authorizer* authz) :
    socket_{socket}, writer_(socket), catena::common::Connect(dm, authz) {
    // Parsing fields
    std::unordered_map<std::string, std::string> fields = {
        {"force_connection", ""},
        {"user_agent", ""},
        {"detail_level", ""},
        {"language", ""}
    };
    parseFields(request, fields);
    language_ = fields.at("language");
    if (dlMap_.find(fields.at("detail_level")) != dlMap_.end()) {
        detailLevel_ = dlMap_.at(fields.at("detail_level"));
    } else {
        detailLevel_ = catena::Device_DetailLevel_NONE;
    }
    userAgent_ = fields.at("user_agent");
    forceConnection_ = fields.at("force_connection") == "true";
    // Writing headers.
    catena::exception_with_status status{"", catena::StatusCode::OK};
    objectId_ = objectCounter_++;
    writer_.writeHeaders(status);
    proceed();
}

void API::Connect::proceed() {
    std::cout << "Connect[" << objectId_ << "]: START" << std::endl;
    // kProcess
    // Cancels all open connections if shutdown signal is sent.
    shutdownSignalId_ = shutdownSignal_.connect([this](){
        this->socket_.close();
        this->hasUpdate_ = true;
        this->cv_.notify_one();
    });

    // Waiting for a value set by server to be sent to execute code.
    valueSetByServerId_ = dm_.valueSetByServer.connect([this](const std::string& oid, const IParam* p, const int32_t idx){
        updateResponse(oid, idx, p);
    });

    // Waiting for a value set by client to be sent to execute code.
    valueSetByClientId_ = dm_.valueSetByClient.connect([this](const std::string& oid, const IParam* p, const int32_t idx){
        updateResponse(oid, idx, p);
    });

    // Waiting for a language to be added to execute code.
    languageAddedId_ = dm_.languageAddedPushUpdate.connect([this](const Device::ComponentLanguagePack& l) {
        updateResponse(l);
    });

    // send client a empty update with slot of the device
    catena::PushUpdates populatedSlots;
    populatedSlots.set_slot(dm_.slot());
    writer_.write(populatedSlots);

    std::unique_lock<std::mutex> lock{mtx_, std::defer_lock};

    // kWrite
    while (socket_.is_open()) {
        lock.lock();
        cv_.wait(lock, [this] { return hasUpdate_; });
        hasUpdate_ = false;
        try {
            if (socket_.is_open()) {
                res_.set_slot(dm_.slot());
                writer_.write(res_);
            }
        } catch (...) { // A little scuffed but no idea how else to detect disconnect.
            socket_.close();
        }
        lock.unlock();
    }

    // kFinish
    std::cout << "Connect[" << objectId_ << "]: FINISH" << std::endl;
    shutdownSignal_.disconnect(shutdownSignalId_);
    dm_.valueSetByClient.disconnect(valueSetByClientId_);
    dm_.valueSetByServer.disconnect(valueSetByServerId_);
}
