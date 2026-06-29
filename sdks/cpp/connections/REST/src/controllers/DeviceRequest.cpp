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
        writer_ = std::make_unique<catena::REST::SocketWriter>(socket, context.origin());
    }
    writeConsole_(CallStatus::kCreate, socket_.is_open());

    //slot_ = context_.slot(); // Slots are unimplemented
}

void DeviceRequest::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());
    catena::exception_with_status rc{"", catena::StatusCode::OK};
    st2138::Device unaryDevice{};
    try {
        bool shallowCopy = true; // controls whether shallow copy or deep copy is used
        std::shared_ptr<catena::common::Authorizer> sharedAuthz;
        catena::common::Authorizer* authz;
        IDevice* dm = nullptr;
        // Validating slot number.
        if (context_.slot() < 0 || context_.slot() > 65535) {
            throw catena::exception_with_status("slot number out of range", catena::StatusCode::INVALID_ARGUMENT);
        }
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
            st2138::Device_DetailLevel dl = context_.detailLevel();

            // Getting subscribed oids if dl == SUBSCRIPTIONS.
            if (dl == st2138::Device_DetailLevel_SUBSCRIPTIONS) {
                auto& subscriptionManager = context_.subscriptionManager();
                subscribedOids_ = subscriptionManager.getAllSubscribedOids(*dm);
            }

            // Getting the serializer object.
            serializer_ = dm->getComponentSerializer(*authz, subscribedOids_, dl, shallowCopy);

            // Getting each component and writing to the stream.
            if (serializer_) {
                while (serializer_->hasMore()) {
                    writeConsole_(CallStatus::kWrite, socket_.is_open());
                    st2138::DeviceComponent component{};
                    {
                        std::lock_guard lg(dm->mutex());
                        component = serializer_->getNext();
                    }
                    if (context_.stream()) {
                        // streams are easy, just send the device
                        writer_->sendResponse(rc, component);
                    } else {
                        // unary we have to build up the proper Device message
                        if (component.has_device()) {
                            // THIS IS ASSUMING THAT THE DEVICE COMPONENT IS ALWAYS THE
                            // FIRST COMPONENT IN THE SERIALIZATION
                            unaryDevice = component.device();
                        } else if (component.has_param()) {
                            unaryDevice.mutable_params()->insert({component.param().oid(), component.param().param()});
                        } else if (component.has_shared_constraint()) {
                            unaryDevice.mutable_constraints()->insert({component.shared_constraint().oid(), component.shared_constraint().constraint()});
                        } else if (component.has_menu()) {
                            // split the menu's oid into group and menu
                            std::string menuOid = component.menu().oid();
                            size_t pos = menuOid.find('/');
                            if (pos == std::string::npos) {
                                // TODO decide the appropriate error handling here
                                continue;
                            }
                            std::string group = menuOid.substr(0, pos);
                            std::string menu = menuOid.substr(pos + 1);
                            // make sure the group exists in the device, should be there from the device chunk
                            if (!unaryDevice.menu_groups().contains(group)) {
                                LOG(ERROR) << "Menu group " << group << " not found in device";
                                continue;
                            }
                            unaryDevice.mutable_menu_groups()->at(group).mutable_menus()->insert({menu, component.menu().menu()});
                        } else if (component.has_command()) {
                            unaryDevice.mutable_commands()->insert({component.command().oid(), component.command().command()});
                        } else if (component.has_language_pack()) {
                            unaryDevice.mutable_language_packs()->mutable_packs()->insert({component.language_pack().language(), component.language_pack().language_pack()});
                        }
                    }
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

    if (context_.stream()) {
        // required to send errors.
        writer_->sendResponse(rc);
    } else {
        // send the full response if not streaming, otherwise the last message has already been sent
        writer_->sendResponse(rc, unaryDevice);
    }

    // Writing the final status to the console.
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    LOG(DEBUG) << RESTMethodMap().getForwardMap().at(context_.method())
            << "DeviceRequest[" << objectId_ << "] finished\n";
}
