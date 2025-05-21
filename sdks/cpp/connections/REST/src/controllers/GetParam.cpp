
// connections/REST
#include <controllers/GetParam.h>
using catena::REST::GetParam;

// Initializes the object counter for GetParam to 0.
int GetParam::objectCounter_ = 0;

GetParam::GetParam(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void GetParam::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    catena::exception_with_status rc{"", catena::StatusCode::OK};
    std::unique_ptr<IParam> param = nullptr;
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
        // Getting the param.
        std::lock_guard lg(dm_.mutex());
        param = dm_.getParam("/" + context_.fields("oid"), rc, *authz);
    // ERROR
    } catch (catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Everything went well, writing the parameter.
    if (param && rc.status == catena::StatusCode::OK) {
        // param is a copy, so we don't need to lock the device again.
        catena::DeviceComponent_ComponentParam ans;
        ans.set_oid(param->getOid());
        param->toProto(*ans.mutable_param(), *authz);
        writer_.sendResponse(rc, ans);
    // Error along the way, finish call with error.
    } else {
        writer_.sendResponse(rc);
    }

    // Error response in case something went wrong along the way.
    if (rc.status != catena::StatusCode::OK) {
        writer_.sendResponse(rc);
    }
}

void GetParam::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "GetParam[" << objectId_ << "] finished\n";
}
