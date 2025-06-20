
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

    catena::DeviceComponent_ComponentParam ans;
    catena::exception_with_status rc("", catena::StatusCode::OK);

    try {
        // Creating authorizer.
        std::shared_ptr<catena::common::Authorizer> sharedAuthz;
        catena::common::Authorizer* authz;
        if (context_.authorizationEnabled()) {
            sharedAuthz = std::make_shared<catena::common::Authorizer>(context_.jwsToken());
            authz = sharedAuthz.get();
        } else {
            authz = &catena::common::Authorizer::kAuthzDisabled;
        }
        // Locking device and getting the param.
        std::lock_guard lg(dm_.mutex());
        std::unique_ptr<IParam> param = dm_.getParam(context_.fqoid(), rc, *authz);
        if (rc.status == catena::StatusCode::OK && param) {
            ans.set_oid(param->getOid());
            rc = param->toProto(*ans.mutable_param(), *authz);
        }
    // ERROR
    } catch (const catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (const std::exception& err) {
        rc = catena::exception_with_status(err.what(), catena::StatusCode::INTERNAL);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing answer to client.
    writer_.sendResponse(rc, ans);
}

void GetParam::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "GetParam[" << objectId_ << "] finished\n";
}
