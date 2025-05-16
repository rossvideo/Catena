
// connections/REST
#include <controllers/GetParam.h>
using catena::REST::GetParam;
using catena::common::ParamDescriptor;

// Initializes the object counter for GetParam to 0.
int GetParam::objectCounter_ = 0;

GetParam::GetParam(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, context_{context}, dm_{dm} {
    // Setting writer_ depending on if client wants stream or unary response.
    if (stream_) {
        writer_ = std::make_unique<catena::REST::SSEWriter>(socket, context.origin());
    } else {
        writer_ = std::make_unique<catena::REST::SocketWriter>(socket, context.origin());
    }

    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void GetParam::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    catena::exception_with_status rc("", catena::StatusCode::OK);
    std::unique_ptr<IParam> param;
    // Authorizer objects.
    std::shared_ptr<catena::common::Authorizer> sharedAuthz;
    catena::common::Authorizer* authz;

    try {
        // Creating authorizer.
        if (context_.authorizationEnabled()) {
            sharedAuthz = std::make_shared<catena::common::Authorizer>(context_.jwsToken());
            authz = sharedAuthz.get();
        } else {
            authz = &catena::common::Authorizer::kAuthzDisabled;
        }
        // Getting the top parameter.
        std::lock_guard lg(dm_.mutex());
        param = dm_.getParam("/" + context_.fields("oid"), rc, *authz);
    // ERROR
    } catch (catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Compiling / writing the response if above was successful.
    if (rc.status == catena::StatusCode::OK && param) {
        // Response proto message.
        catena::DeviceComponent_ComponentParam ans;
        // Tracker for parameters still remaining to be processed.
        std::vector<std::pair<const ParamDescriptor*, catena::Param*>> remainingParams;
        // The sub param map of the parameter currently being processed.
        auto subParams = ans.mutable_param()->mutable_params();

        // Adding top parameter to the response (WITH VALUE).
        ans.set_oid(param->getOid());
        param->toProto(*ans.mutable_param(), *authz);
        remainingParams.push_back(std::make_pair(&param->getDescriptor(), ans.mutable_param()));

        // Processing top parameter and its sub parameters.
        while (!remainingParams.empty()) {
            // Getting the next parameter from remainingParams.
            // The param descriptor of the param to process.
            auto currentPD = remainingParams.back().first;
            // The param in the response to update. NULLPTR if streaming.
            auto currentParam = remainingParams.back().second;
            remainingParams.pop_back();

            // Making sure currentPD didn't get randomly deleted or smt.
            if (currentPD) {
                // Adding sub parameters to remainingParams tracker.
                for (auto [oid, pd] : currentPD->getAllSubParams()) {
                    if (authz->readAuthz(*pd)) {
                        if (stream_) {
                            ans.add_sub_params(oid);
                            remainingParams.push_back(std::make_pair(pd, nullptr));
                        } else {
                            remainingParams.push_back(std::make_pair(pd, &(*subParams)[oid]));
                        }
                    }
                }

                // Stream behaviour
                if (stream_) {
                    // Setting and writing new response before clearing for next.
                    ans.set_oid(currentPD->getOid());
                    currentPD->toProto(*ans.mutable_param(), *authz);
                    writer_->sendResponse(rc, ans);
                    ans.Clear();

                // Unary behaviour
                } else {
                    currentPD->toProto(*currentParam, *authz);
                    subParams = currentParam->mutable_params();
                }
            }
        }

        if (!stream_) { // If not streaming, we need to send the final response.
            writer_->sendResponse(rc, ans);
        }
    }

    // Error response in case something went wrong along the way.
    if (rc.status != catena::StatusCode::OK) {
        writer_->sendResponse(rc);
    }
}

void GetParam::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "GetParam[" << objectId_ << "] finished\n";
}
