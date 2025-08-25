
// connections/REST
#include <controllers/GetParam.h>
using catena::REST::GetParam;

// Initializes the object counter for GetParam to 0.
int GetParam::objectCounter_ = 0;

GetParam::GetParam(tcp::socket& socket, ISocketReader& context, SlotMap& dms) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dms_{dms} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
}

void GetParam::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());

    st2138::DeviceComponent_ComponentParam ans;
    catena::exception_with_status rc("", catena::StatusCode::OK);

    try {
        IDevice* dm = nullptr;
        // Getting device at specified slot.
        if (dms_.contains(context_.slot())) {
            dm = dms_.at(context_.slot());
        }

        // Making sure the device exists.
        if (!dm) {
            rc = catena::exception_with_status("device not found in slot " + std::to_string(context_.slot()), catena::StatusCode::NOT_FOUND);

        } else {
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
            std::lock_guard lg(dm->mutex());
            std::unique_ptr<IParam> param = dm->getParam(context_.fqoid(), rc, *authz);
            if (rc.status == catena::StatusCode::OK && param) {
                ans.set_oid(param->getOid());
                rc = param->toProto(*ans.mutable_param(), *authz);
            }
        }
    // ERROR
    } catch (const catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (const std::exception& err) {
        rc = catena::exception_with_status(err.what(), catena::StatusCode::INTERNAL);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }

    // Finishing by writing answer to client and writing the final status to the console.
    writer_.sendResponse(rc, ans);

    writeConsole_(CallStatus::kFinish, socket_.is_open());
    DEBUG_LOG << "GetParam[" << objectId_ << "] finished\n";
}
