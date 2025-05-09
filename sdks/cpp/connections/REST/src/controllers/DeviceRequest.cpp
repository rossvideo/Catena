// connections/REST
#include <controllers/DeviceRequest.h>
#include <ISubscriptionManager.h>
using catena::REST::DeviceRequest;

// Initializes the object counter for Connect to 0.
int DeviceRequest::objectCounter_ = 0;

DeviceRequest::DeviceRequest(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());

    //slot_ = context_.slot(); // Slots are unimplemented
}

void DeviceRequest::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());
    try {
        bool shallowCopy = true; // controls whether shallow copy or deep copy is used
        catena::exception_with_status rc{"", catena::StatusCode::OK};
        std::shared_ptr<catena::common::Authorizer> sharedAuthz;
        catena::common::Authorizer* authz;
        
        // Setting up authorizer object.
        if (context_.authorizationEnabled()) {
            // Authorizer throws an error if invalid jws token so no need to handle rc.
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
            subscribedOids_ = subscriptionManager.getAllSubscribedOids(dm_);
        }

        // Getting the serializer object.
        serializer_ = dm_.getComponentSerializer(*authz, subscribedOids_, dl, shallowCopy);

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
        writer_.finish(catena::Empty(), catena::exception_with_status("", catena::StatusCode::OK));
        
    // ERROR: Write to stream and end call.
    } catch (catena::exception_with_status& err) {
        writer_.finish(catena::Empty(), err);
    } catch (...) {
        catena::exception_with_status err{"Unknown error", catena::StatusCode::UNKNOWN};
        writer_.finish(catena::Empty(), err);
    }
}

void DeviceRequest::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "DeviceRequest[" << objectId_ << "] finished\n";
}