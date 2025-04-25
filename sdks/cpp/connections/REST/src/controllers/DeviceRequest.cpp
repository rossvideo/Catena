// connections/REST
#include <controllers/DeviceRequest.h>
#include <ISubscriptionManager.h>
using catena::REST::DeviceRequest;

// Initializes the object counter for Connect to 0.
int DeviceRequest::objectCounter_ = 0;

DeviceRequest::DeviceRequest(tcp::socket& socket, SocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());

    //slot_ = context_.slot(); // Slots are unimplemented
}

void DeviceRequest::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());
    try {
        // controls whether shallow copy or deep copy is used
        bool shallowCopy = true;
        std::shared_ptr<catena::common::Authorizer> sharedAuthz;
        catena::common::Authorizer* authz;
        if (context_.authorizationEnabled()) {
            sharedAuthz = std::make_shared<catena::common::Authorizer>(context_.jwsToken());
            authz = sharedAuthz.get();
        } else {
            authz = &catena::common::Authorizer::kAuthzDisabled;
        }
               
        auto& subscriptionManager = context_.getSubscriptionManager();
        subscribedOids_ = subscriptionManager.getAllSubscribedOids(dm_);

        // Set the detail level on the device
        dm_.detail_level(context_.detailLevel());

        // If we're in SUBSCRIPTIONS mode, we'll send minimal set if no subscriptions
        if (context_.detailLevel() == catena::Device_DetailLevel_SUBSCRIPTIONS) {
            if (subscribedOids_.empty()) {
                serializer_ = dm_.getComponentSerializer(*authz, shallowCopy);
            } else {
                serializer_ = dm_.getComponentSerializer(*authz, subscribedOids_, shallowCopy);
            }
        } else {
            serializer_ = dm_.getComponentSerializer(*authz, subscribedOids_, shallowCopy);
        }

        // Getting each component and writing to the stream.
        while (serializer_->hasMore()) {
            writeConsole_(CallStatus::kWrite, socket_.is_open());
            catena::DeviceComponent component{};
            {
            std::lock_guard lg(dm_.mutex());
            component = serializer_->getNext();
            }
            writer_.write(component);
        }
        writer_.finish();
        
    // ERROR: Write to stream and end call.
    } catch (catena::exception_with_status& err) {
        writer_.write(err);
        writer_.finish();
    } catch (...) {
        catena::exception_with_status err{"Unknown error", catena::StatusCode::UNKNOWN};
        writer_.write(err);
        writer_.finish();
    }
}

void DeviceRequest::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    writer_.finish();
    std::cout << "DeviceRequest[" << objectId_ << "] finished\n";
}