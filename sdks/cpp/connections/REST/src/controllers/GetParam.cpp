
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

    // Creating authorizer.
    std::shared_ptr<catena::common::Authorizer> sharedAuthz;
    catena::common::Authorizer* authz;
    if (context_.authorizationEnabled()) {
        sharedAuthz = std::make_shared<catena::common::Authorizer>(context_.jwsToken());
        authz = sharedAuthz.get();
    } else {
        authz = &catena::common::Authorizer::kAuthzDisabled;
    }

    // std::lock_guard lg(dm_.mutex());
    std::unique_ptr<IParam> param = dm_.getParam("/" + context_.fields("oid"), rc, *authz);

    if (rc.status == catena::StatusCode::OK && param) {
        // Variables
        catena::DeviceComponent_ComponentParam ans;
        std::vector<std::pair<const ParamDescriptor*, catena::Param*>> remainingParams;
        auto subParams = ans.mutable_param()->mutable_params();

        // Adding top level parameter to the response.
        ans.set_oid(param->getOid());
        param->toProto(*ans.mutable_param(), *authz);

        remainingParams.push_back(std::make_pair(&param->getDescriptor(), ans.mutable_param()));

        while (!remainingParams.empty()) {
            // Getting the next parameter from remainingParams.
            auto currentPD = remainingParams.back().first;
            auto currentParam = remainingParams.back().second;
            remainingParams.pop_back();

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
                // Setting up new answer.
                ans.set_oid(currentPD->getOid());
                currentPD->toProto(*ans.mutable_param(), *authz);
                // Writing and cleaning answer.
                writer_->sendResponse(rc, ans);
                ans.Clear();
            // Unary behaviour
            } else {
                currentPD->toProto(*currentParam, *authz);
                subParams = currentParam->mutable_params();
            }
        }

        if (!stream_) { // If not streaming, we need to send the final response.
            writer_->sendResponse(rc, ans);
        }
    }
    if (rc.status != catena::StatusCode::OK) {
        // If there was an error, send the error response.
        writer_->sendResponse(rc);
    }
}

void GetParam::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "GetParam[" << objectId_ << "] finished\n";
}
