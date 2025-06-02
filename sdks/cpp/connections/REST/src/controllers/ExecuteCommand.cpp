// connections/REST
#include <controllers/ExecuteCommand.h>
#include <Status.h>
#include <google/protobuf/util/json_util.h>
#include <iostream>

using catena::REST::ExecuteCommand;
using catena::common::IParam;
using catena::common::IParamDescriptor;

// Initializes the object counter for ExecuteCommand to 0.
int ExecuteCommand::objectCounter_ = 0;

ExecuteCommand::ExecuteCommand(tcp::socket& socket, ISocketReader& context, IDevice& dm) :
    socket_{socket}, writer_{socket, context.origin()}, context_{context}, dm_{dm} {
    objectId_ = objectCounter_++;
    writeConsole_(CallStatus::kCreate, socket_.is_open());
    
    try {
        // Initialize the request
        req_.set_slot(context_.slot());
        req_.set_oid(context_.fqoid());
        req_.set_respond(context_.hasField("respond"));
        req_.set_proceed(context_.hasField("proceed"));

        // Parse JSON body if present
        if (!context_.jsonBody().empty()) {
            catena::ExecuteCommandPayload json_payload;
            auto status = google::protobuf::util::JsonStringToMessage(
                absl::string_view(context_.jsonBody()), 
                &json_payload
            );
            if (status.ok() && json_payload.has_value()) {
                *req_.mutable_value() = json_payload.value();
            } else {
                throw catena::exception_with_status("Failed to parse JSON body", catena::StatusCode::INVALID_ARGUMENT);
            }
        }
    } catch (...) {
        catena::exception_with_status err("Failed to parse fields", catena::StatusCode::INVALID_ARGUMENT);
        writer_.sendResponse(err);
    }
}

void ExecuteCommand::proceed() {
    writeConsole_(CallStatus::kProcess, socket_.is_open());
    { // rc scope
    catena::exception_with_status rc("", catena::StatusCode::OK);
    try {
        // Get the command and execute it
        std::unique_ptr<IParam> command = nullptr;
        
        if (context_.authorizationEnabled()) {
            catena::common::Authorizer authz{context_.jwsToken()};
            command = dm_.getCommand(req_.oid(), rc, authz);
        } else {
            command = dm_.getCommand(req_.oid(), rc, catena::common::Authorizer::kAuthzDisabled);
        }

        // If the command is not found, return an error
        if (command != nullptr) {
            // Execute the command and write response if respond = true.
            std::unique_ptr<IParamDescriptor::ICommandResponder> responder = command->executeCommandNew(req_.value());
            while (responder->hasMore()) {
                writeConsole_(CallStatus::kWrite, socket_.is_open());
                catena::CommandResponse res = responder->getNext();
                if (req_.respond()) {
                    writer_.sendResponse(rc, res);
                }
            }
        }
    } catch (catena::exception_with_status& err) {
        rc = catena::exception_with_status(err.what(), err.status);
    } catch (...) {
        rc = catena::exception_with_status("Unknown error", catena::StatusCode::UNKNOWN);
    }
    // Writing final code if respond = false or an error occured.
    if (rc.status != catena::StatusCode::OK || !req_.respond()) {
        writer_.sendResponse(rc);
    }
    }
}

void ExecuteCommand::finish() {
    writeConsole_(CallStatus::kFinish, socket_.is_open());
    std::cout << "ExecuteCommand[" << objectId_ << "] finished\n";
} 
