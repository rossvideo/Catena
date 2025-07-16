// connections/REST
#include <controllers/DeviceRequest.h>
#include <ISubscriptionManager.h>
using catena::REST::DeviceRequest;

// Initializes the object counter for Connect to 0.
int DeviceRequest::objectCounter_ = 0;

DeviceRequest::DeviceRequest(tcp::socket& socket, ISocketReader& context, SlotMap& dms) :
    socket_{socket}, context_{context}, dms_{dms} {
    objectId_ = objectCounter_++;
    // Initializing the writer depending on if the response is stream or unary.
    if (context.stream()) {
        writer_ = std::make_unique<catena::REST::SSEWriter>(socket, context.origin());
    } else {
        writer_ = std::make_unique<catena::REST::SocketWriter>(socket, context.origin(), true);
    }
    writeConsole_(CallStatus::kCreate, socket_.is_open());

    //slot_ = context_.slot(); // Slots are unimplemented
}

void DeviceRequest::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    try {
        bool shallowCopy = true; // controls whether shallow copy or deep copy is used
        std::shared_ptr<catena::common::Authorizer> sharedAuthz;
        catena::common::Authorizer* authz;
        IDevice* dm = nullptr;
        // Getting device at specified slot.
        if (dms_.contains(context_.slot())) {
            dm = dms_.at(context_.slot());
        }
        // Making sure the device exists.
        if (!dm) {
            rc = catena::exception_with_status("device not found in slot " + std::to_string(context_.slot()), catena::StatusCode::NOT_FOUND);

        } else {
            // Setting up authorizer object.
            if (context_.authorizationEnabled()) {
                // Authorizer throws an error if invalid jws token
                sharedAuthz = std::make_shared<catena::common::Authorizer>(context_.jwsToken());
                authz = sharedAuthz.get();
            } else {
                authz = &catena::common::Authorizer::kAuthzDisabled;
            }

            // req_.detail_level defaults to FULL
            catena::Device_DetailLevel dl = context_.detailLevel();

            // Getting subscribed oids if dl == SUBSCRIPTIONS.
            if (dl == catena::Device_DetailLevel_SUBSCRIPTIONS) {
                auto& subscriptionManager = context_.getSubscriptionManager();
                subscribedOids_ = subscriptionManager.getAllSubscribedOids(*dm);
            }

            // Getting the serializer object.
            serializer_ = dm->getComponentSerializer(*authz, subscribedOids_, dl, shallowCopy);

            // Getting each component and writing to the stream.
            if (serializer_) {
                while (serializer_->hasMore()) {
                    writeConsole_(CallStatus::kWrite, socket_.is_open());
                    catena::DeviceComponent component{};
                    {
                        std::lock_guard lg(dm->mutex());
                        component = serializer_->getNext();
                    }
                    writer_->sendResponse(rc, component);
                }
            } else {
                rc = catena::exception_with_status{"Illegal state", catena::StatusCode::INTERNAL};
            }
        }
    // ERROR: Update rc.
    } catch (catena::exception_with_status& err) {
        rc = catena::exception_with_status{err.what(), err.status};
    } catch (const std::exception& e) {
        rc = catena::exception_with_status{std::string("Device request failed: ") + e.what(), 
                                          catena::StatusCode::INTERNAL};
    } catch (...) {
        rc = catena::exception_with_status{"Unknown error", catena::StatusCode::UNKNOWN};
    }
    // empty msg signals unary to send response. Does nothing for stream.
    writer_->sendResponse(rc);

    // Writing the final status to the console.
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    DEBUG_LOG << "DeviceRequest[" << objectId_ << "] finished\n";
}