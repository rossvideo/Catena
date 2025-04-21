// connections/REST
#include <controllers/DeviceRequest.h>
using catena::REST::DeviceRequest;

// Initializes the object counter for Connect to 0.
int DeviceRequest::objectCounter_ = 0;

DeviceRequest::DeviceRequest(tcp::socket& socket, SocketReader& context, Device& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole(CallStatus::kCreate, socket_.is_open());
}

void DeviceRequest::proceed() {
    writeConsole(CallStatus::kProcess, socket_.is_open());
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

        // Get service subscriptions from the manager
        auto* service = context_.getService();
        if (!service) {
            throw catena::exception_with_status("Service not available", catena::StatusCode::INTERNAL);
        }
        auto& subscriptionManager = context_.getSubscriptionManager();
        subscribedOids_ = subscriptionManager.getAllSubscribedOids(dm_);
        
        // If this request has subscriptions, add them
        if (!requestSubscriptions_.empty()) {
            // Add new subscriptions to both the manager and our tracking list
            for (const auto& oid : requestSubscriptions_) {
                catena::exception_with_status rc{"", catena::StatusCode::OK};
                if (!subscriptionManager.addSubscription(oid, dm_, rc)) {
                    throw catena::exception_with_status(std::string("Failed to add subscription: ") + rc.what(), rc.status);
                }
            }
        }

        // Get final list of subscriptions for this response
        subscribedOids_ = subscriptionManager.getAllSubscribedOids(dm_);

        // Set the detail level on the device
        catena::Device_DetailLevel detailLevel_ = catena::Device_DetailLevel_NONE; //Going to fix this in a seperate branch!

        // If we're in SUBSCRIPTIONS mode, we'll send minimal set if no subscriptions
        if (detailLevel_ == catena::Device_DetailLevel_SUBSCRIPTIONS) {
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
            writeConsole(CallStatus::kWrite, socket_.is_open());
            catena::DeviceComponent component{};
            {
                Device::LockGuard lg(dm_);
                component = serializer_->getNext();
            }
            writer_.write(component);
        }
    // ERROR: Write to stream and end call.
    } catch (catena::exception_with_status& err) {
        writer_.write(err);
    } catch (...) {
        catena::exception_with_status err{"Unknown error", catena::StatusCode::UNKNOWN};
        writer_.write(err);
    }
}

void DeviceRequest::finish() {
    writeConsole(CallStatus::kFinish, socket_.is_open());
    writer_.finish();
    std::cout << "DeviceRequest[" << objectId_ << "] finished\n";
}
